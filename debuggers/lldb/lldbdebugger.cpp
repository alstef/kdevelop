/*
 * Low level LLDB interface.
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

#include "lldbdebugger.h"

#include "dbgglobal.h"
#include "debuglog.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShell>

#include <QApplication>
#include <QFileInfo>
#include <QUrl>

using namespace KDevMI::LLDB;
using namespace KDevMI::MI;

LldbDebugger::LldbDebugger(QObject* parent)
    : MIDebugger(parent)
{
}

LldbDebugger::~LldbDebugger()
{
}

void LldbDebugger::start(KConfigGroup& config, const QStringList& extraArguments)
{
    QUrl lldbUrl = config.readEntry(lldbPathEntry, QUrl());
    if (!lldbUrl.isValid() || !lldbUrl.isLocalFile()) {
        debuggerBinary_ = "lldb-mi";
    } else {
        debuggerBinary_ = lldbUrl.toLocalFile();
    }

    QStringList arguments = extraArguments;
    //arguments << "-quiet";

    // TODO: Use executable, argument and environment group settings instead, and corresponding
    // modification to config page
    process_->setProgram(debuggerBinary_, arguments);
    process_->start();

    qCDebug(DEBUGGERLLDB) << "Starting LLDB with command" << debuggerBinary_ + ' ' + arguments.join(' ');
    qCDebug(DEBUGGERLLDB) << "LLDB process pid:" << process_->pid();
    emit userCommandOutput(debuggerBinary_ + ' ' + arguments.join(' ') + '\n');
}
