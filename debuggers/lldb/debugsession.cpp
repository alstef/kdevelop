/*
 * LLDB Debugger Support
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

#include "debugsession.h"

#include "dbgglobal.h"
#include "debuglog.h"
#include "lldbcommand.h"
#include "mi/micommand.h"
#include "stty.h"

#include <debugger/breakpoint/breakpoint.h>
#include <debugger/breakpoint/breakpointmodel.h>
#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/ilaunchconfiguration.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShell>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QGuiApplication>

using namespace KDevMI::LLDB;
using namespace KDevMI::MI;
using namespace KDevelop;

DebugSession::DebugSession()
    : MIDebugSession()
    , m_breakpointController(nullptr)
    , m_variableController(nullptr)
    , m_frameStackModel(nullptr)
{
    m_breakpointController = new BreakpointController(this);
    m_variableController = new VariableController(this);
    m_frameStackModel = new LldbFrameStackModel(this);
}

DebugSession::~DebugSession()
{
}

BreakpointController *DebugSession::breakpointController() const
{
    return m_breakpointController;
}

VariableController *DebugSession::variableController() const
{
    return m_variableController;
}

LldbFrameStackModel *DebugSession::frameStackModel() const
{
    return m_frameStackModel;
}

LldbDebugger *DebugSession::createDebugger() const
{
    return new LldbDebugger;
}

MICommand *DebugSession::createCommand(MI::CommandType type, const QString& arguments,
                                       MI::CommandFlags flags) const
{
    return new LldbCommand(type, arguments, flags);
}

void DebugSession::initializeDebugger()
{
    //addCommand(MI::EnableTimings, "yes");

    // TODO: lldb data formatter
    /*
    QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                              "kdevlldb/printers/lldbinit");
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString quotedPrintersPath = fileInfo.dir().path()
                                             .replace('\\', "\\\\")
                                             .replace('"', "\\\"");
        queueCmd(new MICommand(MI::NonMI,
            QString("python sys.path.insert(0, \"%0\")").arg(quotedPrintersPath)));
        queueCmd(new MICommand(MI::NonMI, "source " + fileName));
    }
    */

    qCDebug(DEBUGGERLLDB) << "Initialized LLDB";
}

void DebugSession::configure(ILaunchConfiguration *cfg)
{
    // Read Configuration values
    KConfigGroup grp = cfg->config();
    bool displayStaticMembers = grp.readEntry(KDevMI::staticMembersEntry, false);
    bool asmDemangle = grp.readEntry(KDevMI::demangleNamesEntry, true);
    QUrl configLldbScript = grp.readEntry(KDevMI::customLldbConfigEntry, QUrl());

    // break on start: can't use "-exec-run --start" because in lldb-mi
    // the inferior stops without any notification
    bool breakOnStart = grp.readEntry(KDevMI::breakOnStartEntry, false);
    if (breakOnStart) {
        BreakpointModel* m = ICore::self()->debugController()->breakpointModel();
        bool found = false;
        foreach (Breakpoint *b, m->breakpoints()) {
            if (b->location() == "main") {
                found = true;
                break;
            }
        }
        if (!found) {
            m->addCodeBreakpoint("main");
        }
    }

    // Needed so that breakpoint widget has a chance to insert breakpoints.
    // FIXME: a bit hacky, as we're really not ready for new commands.
    setDebuggerStateOn(s_dbgBusy);
    raiseEvent(debugger_ready);

    // custom config script
    if (configLldbScript.isValid()) {
        addCommand(MI::NonMI, "command source -s 0" + KShell::quoteArg(configLldbScript.toLocalFile()));
    }

    qCDebug(DEBUGGERGDB) << "Per inferior configuration done";
}

bool DebugSession::execInferior(ILaunchConfiguration *cfg, const QString &executable)
{
    qCDebug(DEBUGGERGDB) << "Executing inferior";

    // debugger specific config
    configure(cfg);

    // config that can't be placed in configure()
    KConfigGroup grp = cfg->config();

    // TODO: remote debugging for lldb, see lldb-mi file-exec-and-symbols extension
    /*
    QUrl runShellScript = grp.readEntry(KDevMI::remoteGdbShellEntry, QUrl());
    QUrl runGdbScript = grp.readEntry(KDevMI::remoteGdbRunEntry, QUrl());
    // FIXME: have a check box option that controls remote debugging
    if (runShellScript.isValid()) {
        // Special for remote debug, the remote inferior is started by this shell script
        QByteArray tty(m_tty->getSlave().toLatin1());
        QByteArray options = QByteArray(">") + tty + QByteArray("  2>&1 <") + tty;

        QProcess *proc = new QProcess;
        QStringList arguments;
        arguments << "-c" << KShell::quoteArg(runShellScript.toLocalFile()) +
            ' ' + KShell::quoteArg(executable) + QString::fromLatin1(options);

        qCDebug(DEBUGGERGDB) << "starting sh" << arguments;
        proc->start("sh", arguments);
        //PORTING TODO QProcess::DontCare);
    }

    if (runGdbScript.isValid()) {
        // Special for remote debug, gdb script at run is requested, to connect to remote inferior

        // Race notice: wait for the remote gdbserver/executable
        // - but that might be an issue for this script to handle...

        // Note: script could contain "run" or "continue"

        // Future: the shell script should be able to pass info (like pid)
        // to the gdb script...

        queueCmd(new SentinelCommand([this, runGdbScript]() {
            breakpointController()->initSendBreakpoints();

            breakpointController()->setDeleteDuplicateBreakpoints(true);
            qCDebug(DEBUGGERGDB) << "Running gdb script " << KShell::quoteArg(runGdbScript.toLocalFile());

            queueCmd(new MICommand(MI::NonMI, "source " + KShell::quoteArg(runGdbScript.toLocalFile()),
                                    [this](const MI::ResultRecord&) {
                                        breakpointController()->setDeleteDuplicateBreakpoints(false);
                                    },
                                    CmdMaybeStartsRunning));
            raiseEvent(connected_to_program);
        }, CmdMaybeStartsRunning));
    } else {
    */

    // normal local debugging
    addCommand(MI::FileExecAndSymbols, KShell::quoteArg(executable),
               this, &DebugSession::handleFileExecAndSymbols,
               CmdHandlesError);
    raiseEvent(connected_to_program);

    addCommand(new SentinelCommand([this]() {
        breakpointController()->initSendBreakpoints();
        addCommand(MI::ExecRun, QString(), CmdMaybeStartsRunning);
    }, CmdMaybeStartsRunning));
    return true;
}

void DebugSession::handleFileExecAndSymbols(const MI::ResultRecord& r)
{
    if (r.reason == "error") {
        KMessageBox::error(
            qApp->activeWindow(),
            i18n("<b>Could not start debugger:</b><br />")+
            r["msg"].literal(),
            i18n("Startup error"));
        stopDebugger();
    }
}
