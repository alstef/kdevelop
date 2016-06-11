/*
 * GDB Debugger Support
 *
 * Copyright 2003 John Birch <jbb@kdevelop.org>
 * Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "gdboutputwidget.h"

#include "dbgglobal.h"
#include "debuggerplugin.h"
#include "debuglog.h"
#include "debugsession.h"

#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>

#include <KColorScheme>
#include <KComboBox>
#include <KHistoryComboBox>
#include <KLocalizedString>

#include <QApplication>
#include <QClipboard>
#include <QFocusEvent>
#include <QMenu>
#include <QLabel>
#include <QLayout>
#include <QToolButton>
#include <QToolTip>
#include <QScrollBar>
#include <QScopedPointer>

using namespace KDevMI::GDB;

/***************************************************************************/

GDBOutputWidget::GDBOutputWidget(CppDebuggerPlugin* plugin, QWidget *parent) :
    QWidget(parent),
    m_userGDBCmdEditor(0),
    m_Interrupt(0),
    m_gdbView(0),
    showInternalCommands_(false),
    maxLines_(5000)
{
    setWindowIcon(QIcon::fromTheme("dialog-scripts", windowIcon()));
    setWindowTitle(i18n("GDB Output"));
    setWhatsThis(i18n("<b>GDB output</b><p>"
                    "Shows all gdb commands being executed. "
                    "You can also issue any other gdb command while debugging.</p>"));

    m_gdbView = new OutputTextEdit(this);
    m_gdbView->setReadOnly(true);

    m_userGDBCmdEditor = new KHistoryComboBox (this);

    QLabel *label = new QLabel(i18n("&GDB cmd:"), this);
    label->setBuddy(m_userGDBCmdEditor);

    m_Interrupt = new QToolButton( this );
    m_Interrupt->setIcon ( QIcon::fromTheme( "media-playback-pause" ) );
    m_Interrupt->setToolTip( i18n ( "Pause execution of the app to enter gdb commands" ) );

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->addWidget(m_gdbView);
    topLayout->setStretchFactor(m_gdbView, 1);
    topLayout->setMargin(0);

    QBoxLayout *userGDBCmdEntry = new QHBoxLayout();
    userGDBCmdEntry->addWidget(label);
    userGDBCmdEntry->addWidget(m_userGDBCmdEditor);
    userGDBCmdEntry->setStretchFactor(m_userGDBCmdEditor, 1);
    userGDBCmdEntry->addWidget(m_Interrupt);
    topLayout->addLayout(userGDBCmdEntry);

    setLayout(topLayout);

    slotStateChanged(s_none, s_dbgNotStarted);

    connect(m_userGDBCmdEditor, static_cast<void(KHistoryComboBox::*)()>(&KHistoryComboBox::returnPressed),
            this, &GDBOutputWidget::slotGDBCmd);
    connect(m_Interrupt, &QToolButton::clicked, this, &GDBOutputWidget::breakInto);

    updateTimer_.setSingleShot(true);
    connect(&updateTimer_, &QTimer::timeout,
             this, &GDBOutputWidget::flushPending);

    connect(KDevelop::ICore::self()->debugController(), &KDevelop::IDebugController::currentSessionChanged,
            this, &GDBOutputWidget::currentSessionChanged);

    connect(plugin, &CppDebuggerPlugin::reset, this, &GDBOutputWidget::clear);
    connect(plugin, &CppDebuggerPlugin::raiseDebuggerConsoleViews, this, &GDBOutputWidget::requestRaise);

    currentSessionChanged(KDevelop::ICore::self()->debugController()->currentSession());

//     TODO Port to KF5
//     connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
//             this, SLOT(updateColors()));
    updateColors();

}

void GDBOutputWidget::updateColors()
{
    KColorScheme scheme(QPalette::Active);
    gdbColor_ = scheme.foreground(KColorScheme::LinkText).color();
    errorColor_ = scheme.foreground(KColorScheme::NegativeText).color();
}

void GDBOutputWidget::currentSessionChanged(KDevelop::IDebugSession* s)
{
    DebugSession *session = qobject_cast<DebugSession*>(s);
    if (!session) return;
     connect(this, &GDBOutputWidget::userGDBCmd,
             session, &DebugSession::addUserCommand);
     connect(this, &GDBOutputWidget::breakInto,
             session, &DebugSession::interruptDebugger);

     connect(session, &DebugSession::debuggerInternalCommandOutput,
             this, &GDBOutputWidget::slotInternalCommandStdout);
     connect(session, &DebugSession::debuggerUserCommandOutput,
             this, &GDBOutputWidget::slotUserCommandStdout);

     connect(session, &DebugSession::debuggerStateChanged,
             this, &GDBOutputWidget::slotStateChanged);

     slotStateChanged(s_none, session->debuggerState());
}


/***************************************************************************/

GDBOutputWidget::~GDBOutputWidget()
{
    delete m_gdbView;
    delete m_userGDBCmdEditor;
}

/***************************************************************************/

void GDBOutputWidget::clear()
{
    if (m_gdbView)
        m_gdbView->clear();

    userCommands_.clear();
    allCommands_.clear();
}

/***************************************************************************/

void GDBOutputWidget::slotInternalCommandStdout(const QString& line)
{
    newStdoutLine(line, true);
}

void GDBOutputWidget::slotUserCommandStdout(const QString& line)
{
    qCDebug(DEBUGGERGDB) << "User command stdout: " << line;
    newStdoutLine(line, false);
}

namespace {
    QString colorify(QString text, const QColor& color)
    {
        // Make sure the newline is at the end of the newly-added
        // string. This is so that we can always correctly remove
        // newline inside 'flushPending'.
        if (!text.endsWith('\n'))
          text.append('\n');

        if (text.endsWith('\n'))
        {
            text.remove(text.length()-1, 1);
        }
        text = "<font color=\"" + color.name() +  "\">" + text + "</font><br>";
        return text;
    }
}


void GDBOutputWidget::newStdoutLine(const QString& line,
                                    bool internal)
{
    QString s = line.toHtmlEscaped();
    if (s.startsWith("(gdb)"))
    {
        s = colorify(s, gdbColor_);
    }
    else
        s.replace('\n', "<br>");

    allCommands_.append(s);
    allCommandsRaw_.append(line);
    trimList(allCommands_, maxLines_);
    trimList(allCommandsRaw_, maxLines_);

    if (!internal)
    {
        userCommands_.append(s);
        userCommandsRaw_.append(line);
        trimList(userCommands_, maxLines_);
        trimList(userCommandsRaw_, maxLines_);
    }

    if (!internal || showInternalCommands_)
        showLine(s);
}


void GDBOutputWidget::showLine(const QString& line)
{
    pendingOutput_ += line;

    // To improve performance, we update the view after some delay.
    if (!updateTimer_.isActive())
    {
        updateTimer_.start(100);
    }
}

void GDBOutputWidget::trimList(QStringList& l, int max_size)
{
    int length = l.count();
    if (length > max_size)
    {
        for(int to_delete = length - max_size; to_delete; --to_delete)
        {
            l.erase(l.begin());
        }
    }
}

void GDBOutputWidget::setShowInternalCommands(bool show)
{
    if (show != showInternalCommands_)
    {
        showInternalCommands_ = show;

        // Set of strings to show changes, text edit still has old
        // set. Refresh.
        m_gdbView->clear();
        QStringList& newList =
            showInternalCommands_ ? allCommands_ : userCommands_;

        QStringList::iterator i = newList.begin(), e = newList.end();
        for(; i != e; ++i)
        {
            // Note that color formatting is already applied to '*i'.
            showLine(*i);
        }
    }
}

/***************************************************************************/

void GDBOutputWidget::slotReceivedStderr(const char* line)
{
    QString colored = colorify(QString::fromLatin1(line).toHtmlEscaped(), errorColor_);
    // Errors are shown inside user commands too.
    allCommands_.append(colored);
    trimList(allCommands_, maxLines_);
    userCommands_.append(colored);
    trimList(userCommands_, maxLines_);

    allCommandsRaw_.append(line);
    trimList(allCommandsRaw_, maxLines_);
    userCommandsRaw_.append(line);
    trimList(userCommandsRaw_, maxLines_);

    showLine(colored);
}

/***************************************************************************/

void GDBOutputWidget::slotGDBCmd()
{
    QString GDBCmd(m_userGDBCmdEditor->currentText());
    if (!GDBCmd.isEmpty())
    {
        m_userGDBCmdEditor->addToHistory(GDBCmd);
        m_userGDBCmdEditor->clearEditText();
        emit userGDBCmd(GDBCmd);
    }
}

void GDBOutputWidget::flushPending()
{
    m_gdbView->setUpdatesEnabled(false);

    // QTextEdit adds newline after paragraph automatically.
    // So, remove trailing newline to avoid double newlines.
    if (pendingOutput_.endsWith('\n'))
        pendingOutput_.remove(pendingOutput_.length()-1, 1);
    Q_ASSERT(!pendingOutput_.endsWith('\n'));

    QTextDocument *document = m_gdbView->document();
    QTextCursor cursor(document);
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(pendingOutput_);
    pendingOutput_ = "";

    m_gdbView->verticalScrollBar()->setValue(m_gdbView->verticalScrollBar()->maximum());
    m_gdbView->setUpdatesEnabled(true);
    m_gdbView->update();
    if (m_cmdEditorHadFocus) {
        m_userGDBCmdEditor->setFocus();
    }
}

/***************************************************************************/

void GDBOutputWidget::slotStateChanged(DBGStateFlags oldStatus, DBGStateFlags newStatus)
{
    Q_UNUSED(oldStatus)
    if (newStatus & s_dbgNotStarted)
    {
        m_Interrupt->setEnabled(false);
        m_userGDBCmdEditor->setEnabled(false);
        return;
    }
    else
    {
        m_Interrupt->setEnabled(true);
    }

    if (newStatus & s_dbgBusy)
    {
        if (m_userGDBCmdEditor->isEnabled()) {
            m_cmdEditorHadFocus = m_userGDBCmdEditor->hasFocus();
        }
        m_userGDBCmdEditor->setEnabled(false);
    }
    else
    {
        m_userGDBCmdEditor->setEnabled(true);
    }
}

/***************************************************************************/

void GDBOutputWidget::focusInEvent(QFocusEvent */*e*/)
{
    m_gdbView->verticalScrollBar()->setValue(m_gdbView->verticalScrollBar()->maximum());
    m_userGDBCmdEditor->setFocus();
}

void GDBOutputWidget::savePartialProjectSession()
{
    KConfigGroup config(KSharedConfig::openConfig(), "GDB Debugger");

    config.writeEntry("showInternalCommands", showInternalCommands_);
}

void GDBOutputWidget::restorePartialProjectSession()
{
    KConfigGroup config(KSharedConfig::openConfig(), "GDB Debugger");

    showInternalCommands_ = config.readEntry("showInternalCommands", false);
}


void GDBOutputWidget::contextMenuEvent(QContextMenuEvent * e)
{
    QScopedPointer<QMenu> popup(new QMenu(this));

    QAction* action = popup->addAction(i18n("Show Internal Commands"),
                               this,
                               SLOT(toggleShowInternalCommands()));

    action->setCheckable(true);
    action->setChecked(showInternalCommands_);
    action->setWhatsThis(i18n(
            "Controls if commands issued internally by KDevelop "
            "will be shown or not.<br>"
            "This option will affect only future commands, it will not "
            "add or remove already issued commands from the view."));

    popup->addAction(i18n("Copy All"),
                      this,
                      SLOT(copyAll()));

    popup->exec(e->globalPos());
}

void GDBOutputWidget::copyAll()
{
    /* See comments for allCommandRaw_ for explanations of
       this complex logic, as opposed to calling text(). */
    const QStringList& raw = showInternalCommands_ ?
        allCommandsRaw_ : userCommandsRaw_;
    QString text;
    for (int i = 0; i < raw.size(); ++i)
        text += raw.at(i);

    // Make sure the text is pastable both with Ctrl-C and with
    // middle click.
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    QApplication::clipboard()->setText(text, QClipboard::Selection);
}

void GDBOutputWidget::toggleShowInternalCommands()
{
    setShowInternalCommands(!showInternalCommands_);
}


OutputTextEdit::OutputTextEdit(GDBOutputWidget * parent)
    : QTextEdit(parent)
{
}

void OutputTextEdit::contextMenuEvent(QContextMenuEvent * event)
{
    QMenu* popup = createStandardContextMenu();

    QAction* action = popup->addAction(i18n("Show Internal Commands"),
                               parent(),
                               SLOT(toggleShowInternalCommands()));

    action->setCheckable(true);
    action->setChecked(static_cast<GDBOutputWidget*>(parent())->showInternalCommands());
    action->setWhatsThis(i18n(
            "Controls if commands issued internally by KDevelop "
            "will be shown or not.<br>"
            "This option will affect only future commands, it will not "
            "add or remove already issued commands from the view."));

    popup->exec(event->globalPos());
}

bool GDBOutputWidget::showInternalCommands() const
{
    return showInternalCommands_;
}
