/*
 * Common code for MI debugger support
 *
 * Copyright 1999-2001 John Birch <jbb@kdevelop.org>
 * Copyright 2001 by Bernd Gehrmann <bernd@kdevelop.org>
 * Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2016  Aetf <aetf@unlimitedcodeworks.xyz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "midebuggerplugin.h"

#include <execute/iexecuteplugin.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/launchconfigurationtype.h>
#include <language/interfaces/editorcontext.h>
#include <sublime/view.h>

#include <KActionCollection>
#include <KLocalizedString>
#include <KParts/MainWindow>
#include <KStringHandler>

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QSignalMapper>
#include <QTimer>

using namespace KDevelop;
using namespace KDevMI;

namespace {

template<class T>
class DebuggerToolFactory : public IToolViewFactory
{
public:
  DebuggerToolFactory(MIDebuggerPlugin * plugin, const QString &id, Qt::DockWidgetArea defaultArea)
  : m_plugin(plugin), m_id(id), m_defaultArea(defaultArea)
  {}

  QWidget* create(QWidget *parent = 0) override
  {
      return new T(m_plugin, parent);
  }

  QString id() const override
  {
      return m_id;
  }

  Qt::DockWidgetArea defaultPosition() override
  {
      return m_defaultArea;
  }

  void viewCreated(Sublime::View* view) override
  {
      if (view->widget()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("requestRaise()")) != -1)
          QObject::connect(view->widget(), SIGNAL(requestRaise()), view, SLOT(requestRaise()));
  }

  /* At present, some debugger widgets (e.g. breakpoint) contain actions so that shortcuts
     work, but they don't need any toolbar.  So, suppress toolbar action.  */
  QList<QAction*> toolBarActions(QWidget* viewWidget) const override
  {
      Q_UNUSED(viewWidget);
      return QList<QAction*>();
  }

private:
    MIDebuggerPlugin * m_plugin;
    QString m_id;
    Qt::DockWidgetArea m_defaultArea;
};

} // end of anonymous namespace


MIDebuggerPlugin::MIDebuggerPlugin(const QString &componentName, QObject *parent)
    : KDevelop::IPlugin(componentName, parent)
{
    KDEV_USE_EXTENSION_INTERFACE(KDevelop::IStatus)

    core()->debugController()->initializeUi();

    //TODO: setXMLFile in derived class
    //setXMLFile("kdevlldbui.rc");

    setupToolviews();
    setupActions();
    setupDBus();

    // add a debug launcher to each native app configuration
    auto plugins = core()->pluginController()->allPluginsForExtension("org.kdevelop.IExecutePlugin");
    for (auto plugin : plugins) {
        IExecutePlugin* iface = plugin->extension<IExecutePlugin>();
        Q_ASSERT(iface);
        auto type = core()->runController()->launchConfigurationTypeForId(iface->nativeAppConfigTypeId());
        Q_ASSERT(type);
        // TODO: port MILauncher
        //type->addLauncher( new LldbLauncher( this, iface ) );
    }
}

void MIDebuggerPlugin::setupToolviews()
{
    // TODO: port tool views
    /*
    disassemblefactory = new DebuggerToolFactory<DisassembleWidget>(
    this, "org.kdevelop.debugger.DisassemblerView", Qt::BottomDockWidgetArea);

    lldbfactory = new DebuggerToolFactory<LLDBOutputWidget>(
    this, "org.kdevelop.debugger.ConsoleView",Qt::BottomDockWidgetArea);

    core()->uiController()->addToolView(
        i18n("Disassemble/Registers"),
        disassemblefactory);

    core()->uiController()->addToolView(
        i18n("LLDB"),
        lldbfactory);

#ifndef WITH_OKTETA
    memoryviewerfactory = nullptr;
#else
    memoryviewerfactory = new DebuggerToolFactory<MemoryViewerWidget>(
    this, "org.kdevelop.debugger.MemoryView", Qt::BottomDockWidgetArea);
    core()->uiController()->addToolView(
        i18n("Memory"),
        memoryviewerfactory);
#endif
*/
}

void MIDebuggerPlugin::setupActions()
{
    KActionCollection* ac = actionCollection();

    QAction * action = new QAction(this);
    action->setIcon(QIcon::fromTheme("core"));
    action->setText(i18n("Examine Core File..."));
    action->setToolTip(i18n("Examine core file"));
    action->setWhatsThis(i18n("<b>Examine core file</b>"
                              "<p>This loads a core file, which is typically created "
                              "after the application has crashed, e.g. with a "
                              "segmentation fault. The core file contains an "
                              "image of the program memory at the time it crashed, "
                              "allowing you to do a post-mortem analysis.</p>"));
    // TODO: port exam core to KJob
    connect(action, &QAction::triggered, this, &MIDebuggerPlugin::slotExamineCore);
    ac->addAction("debug_core", action);

    #ifdef KDEV_ENABLE_DBG_ATTACH_DIALOG
    action = new QAction(this);
    action->setIcon(QIcon::fromTheme("connect_creating"));
    action->setText(i18n("Attach to Process..."));
    action->setToolTip(i18n("Attach to process"));
    action->setWhatsThis(i18n("<b>Attach to process</b>"
                              "<p>Attaches the debugger to a running process.</p>"));
    // TODO: port attach process to KJob
    connect(action, &QAction::triggered, this, &MIDebuggerPlugin::slotAttachProcess);
    ac->addAction("debug_attach", action);
    #endif
}

void MIDebuggerPlugin::setupDBus()
{
    m_drkonqiMap = new QSignalMapper(this);
    connect(m_drkonqiMap, static_cast<void(QSignalMapper::*)(QObject*)>(&QSignalMapper::mapped),
            this, &MIDebuggerPlugin::slotDebugExternalProcess);

    QDBusConnectionInterface* dbusInterface = QDBusConnection::sessionBus().interface();
    for (const auto &service : dbusInterface->registeredServiceNames().value()) {
        slotDBusServiceRegistered(service);
    }

    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &MIDebuggerPlugin::slotDBusServiceRegistered);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &MIDebuggerPlugin::slotDBusServiceUnregistered);
}

void MIDebuggerPlugin::unload()
{
    // TODO: port tool views
    /*
    core()->uiController()->removeToolView(disassemblefactory);
    core()->uiController()->removeToolView(lldbfactory);
    core()->uiController()->removeToolView(memoryviewerfactory);
    */
}

MIDebuggerPlugin::~MIDebuggerPlugin()
{
}

void MIDebuggerPlugin::slotDBusServiceRegistered(const QString& service)
{
    if (service.startsWith("org.kde.drkonqi")) {
        // New registration
        QDBusInterface* drkonqiInterface = new QDBusInterface(service, "/krashinfo",
                                                              QString(), QDBusConnection::sessionBus(),
                                                              this);
        m_drkonqis.insert(service, drkonqiInterface);

        connect(drkonqiInterface, SIGNAL(acceptDebuggingApplication()), m_drkonqiMap, SLOT(map()));
        m_drkonqiMap->setMapping(drkonqiInterface, drkonqiInterface);

        drkonqiInterface->call("registerDebuggingApplication", i18n("KDevelop"));
    }
}

void MIDebuggerPlugin::slotDBusServiceUnregistered(const QString& service)
{
    if (service.startsWith("org.kde.drkonqi")) {
        // Deregistration
        if (m_drkonqis.contains(service))
            delete m_drkonqis.take(service);
    }
}

void MIDebuggerPlugin::slotDebugExternalProcess(QObject* interface)
{
    auto dbusInterface = static_cast<QDBusInterface*>(interface);

    QDBusReply<int> reply = dbusInterface->call("pid");
    if (reply.isValid()) {
        attachProcess(reply.value());
        QTimer::singleShot(500, this, &MIDebuggerPlugin::slotCloseDrKonqi);

        m_drkonqi = m_drkonqis.key(dbusInterface);
    }

    core()->uiController()->activeMainWindow()->raise();
}

void MIDebuggerPlugin::slotCloseDrKonqi()
{
    if (!m_drkonqi.isEmpty()) {
        QDBusInterface drkonqiInterface(m_drkonqi, "/MainApplication", "org.kde.KApplication");
        drkonqiInterface.call("quit");
        m_drkonqi.clear();
    }
}

ContextMenuExtension MIDebuggerPlugin::contextMenuExtension(Context* context)
{
    ContextMenuExtension menuExt = IPlugin::contextMenuExtension(context);

    if (context->type() != KDevelop::Context::EditorContext)
        return menuExt;

    EditorContext *econtext = dynamic_cast<EditorContext*>(context);
    if (!econtext)
        return menuExt;

    QString contextIdent = econtext->currentWord();

    if (!contextIdent.isEmpty())
    {
        QString squeezed = KStringHandler::csqueeze(contextIdent, 30);

        QAction* action = new QAction(this);
        action->setText(i18n("Evaluate: %1", squeezed));
        action->setWhatsThis(i18n("<b>Evaluate expression</b>"
                                  "<p>Shows the value of the expression under the cursor.</p>"));
        connect(action, &QAction::triggered, this, [this, contextIdent](){
            emit addWatchVariable(contextIdent);
        });
        menuExt.addAction(ContextMenuExtension::DebugGroup, action);

        action = new QAction(this);
        action->setText(i18n("Watch: %1", squeezed));
        action->setWhatsThis(i18n("<b>Watch expression</b>"
                                  "<p>Adds the expression under the cursor to the Variables/Watch list.</p>"));
        connect(action, &QAction::triggered, this, [this, contextIdent](){
            emit evaluateExpression(contextIdent);
        });
        menuExt.addAction(ContextMenuExtension::DebugGroup, action);
    }

    return menuExt;
}

/*
 * TODO: implement this in derived class
MIDebugSession* MIDebuggerPlugin::createSession()
{
    MIDebugSession *session = new DebugSession();
    KDevelop::ICore::self()->debugController()->addSession(session);
    connect(session, &DebugSession::showMessage, this, &MIDebuggerPlugin::controllerMessage);
    connect(session, &DebugSession::reset, this, &MIDebuggerPlugin::reset);
    connect(session, &DebugSession::finished, this, &MIDebuggerPlugin::slotFinished);
    connect(session, &DebugSession::raiseDebuggerConsoleViews,
            this, &MIDebuggerPlugin::raiseLldbConsoleViews);
    return session;
}
*/

void MIDebuggerPlugin::slotExamineCore()
{
    showStatusMessage(i18n("Choose a core file to examine..."), 1000);

    // TODO: port exam core to KJob
    /*
    SelectCoreDialog dlg(KDevelop::ICore::self()->uiController()->activeMainWindow());
    if (dlg.exec() == QDialog::Rejected) {
        return;
    }

    showStatusMessage(i18n("Examining core file %1", dlg.core().toLocalFile()), 1000);

    DebugSession* session = createSession();
    session->examineCoreFile(dlg.binary(), dlg.core());

    KillSessionJob *job = new KillSessionJob(session);
    job->setObjectName(i18n("Debug core file"));
    core()->runController()->registerJob(job);
    job->start();
    */
}

#ifdef KDEV_ENABLE_DBG_ATTACH_DIALOG
void MIDebuggerPlugin::slotAttachProcess()
{
    showStatusMessage(i18n("Choose a process to attach to..."), 1000);

    // TODO: port attach process to KJob
    /*
    ProcessSelectionDialog dlg;
    if (!dlg.exec() || !dlg.pidSelected())
        return;

    int pid = dlg.pidSelected();
    if(QApplication::applicationPid()==pid)
        KMessageBox::error(KDevelop::ICore::self()->uiController()->activeMainWindow(),
                            i18n("Not attaching to process %1: cannot attach the debugger to itself.", pid));
    else
        attachProcess(pid);
    */
}
#endif

void MIDebuggerPlugin::attachProcess(int pid)
{
    showStatusMessage(i18n("Attaching to process %1", pid), 1000);

    // TODO: port attach process to KJob
    /*
    DebugSession* session = createSession();
    session->attachToProcess(pid);

    KillSessionJob *job = new KillSessionJob(session);
    job->setObjectName(i18n("Debug process %1", pid));
    core()->runController()->registerJob(job);
    job->start();
    */
}

QString MIDebuggerPlugin::statusName() const
{
    return i18n("Debugger");
}

void MIDebuggerPlugin::showStatusMessage(const QString& msg, int timeout)
{
    emit showMessage(this, msg, timeout);
}
