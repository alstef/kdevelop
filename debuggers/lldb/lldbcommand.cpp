/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "lldbcommand.h"

using namespace KDevMI::LLDB;
using namespace KDevMI::MI;

LldbCommand::LldbCommand(CommandType type, const QString& arguments, CommandFlags flags)
    : MICommand(type, arguments, flags)
{

}

LldbCommand::~LldbCommand()
{
}

QString LldbCommand::miCommand() const
{
    QString command;
    bool isMI = false;
    // TODO: find alternatives to the following command which are not supported in lldb-mi
    switch(type()) {
        case BreakCommands:
            command = "";
            break;
        case BreakInfo:
            command = "";
            break;
        case BreakList:
            command = "";
            break;
        case BreakWatch:
            command = "break-watch";
            break;

        case DataListChangedRegisters:
            command = "data-list-changed-registers";
            break;
        case DataReadMemory: // not implemented, deprecated
            command = "data-read-memory";
            break;
        case DataWriteRegisterVariables:
            command = "data-write-register-values";
            break;

        case EnablePrettyPrinting: // FIXME: done, supported=0
            command = "enable-pretty-printing";
            break;
        case EnableTimings:
            command = "enable-timings";
            break;

        case EnvironmentDirectory:
            command = "environment-directory";
            break;
        case EnvironmentPath:
            command = "environment-path";
            break;
        case EnvironmentPwd:
            command = "environment-pwd";
            break;

        case ExecUntil:
            // TODO: write test case for this
            command = "thread until";
            break;

        case FileExecFile:
            command = "file-exec-file";//"exec-file"
            break;
        case FileListExecSourceFile:
            command = "file-list-exec-source-file";
            break;
        case FileListExecSourceFiles:
            command = "file-list-exec-source-files";
            break;
        case FileSymbolFile:
            command = "file-symbol-file";//"symbol-file"
            break;

        case GdbVersion:
            command = "gdb-version";//"show version"
            break;

        case InferiorTtyShow:
            command = "inferior-tty-show";
            break;

        case SignalHandle: // FIXME: used by multiple location
            return "handle";
            //command = "signal-handle";
            break;

        case TargetDisconnect:
            command = "target-disconnect";//"disconnect"
            break;
        case TargetDownload:
            command = "target-download";
            break;

        case ThreadListIds:
            command = "thread-list-ids";
            break;
        case ThreadSelect:
            command = "thread-select";
            break;

        case TraceFind:
            command = "trace-find";
            break;
        case TraceStart:
            command = "trace-start";
            break;
        case TraceStop:
            command = "trace-stop";
            break;

        case VarInfoNumChildren:
            command = "var-info-num-children";
            break;
        case VarInfoType:
            command = "var-info-type";
            break;
        case VarSetFrozen:
            command = "var-set-frozen";
            break;
        case VarShowFormat:
            command = "var-show-format";
            break;
        default:
            return MICommand::miCommand();
    }

    if (isMI) {
        command.prepend('-');
    }
    return command;
}
