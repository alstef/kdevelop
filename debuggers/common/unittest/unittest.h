/*
 * Common helpers for MI debugger unit tests
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

#ifndef KDEVMI_UNITTEST_H
#define KDEVMI_UNITTEST_H

#include <debugger/interfaces/idebugsession.h>

#include <QPointer>
#include <QString>
#include <QTime>
#include <QUrl>

namespace KDevMI {

class MIDebugSession;

namespace UnitTest {

QUrl findExecutable(const QString& name);
QString findSourceFile(const char *file, const QString& name);
bool isAttachForbidden(const char *file, int line);

void compareData(QModelIndex index, QString expected, const char *file, int line);

bool waitForState(MIDebugSession *session, KDevelop::IDebugSession::DebuggerState state,
                  const char *file, int line, bool waitForIdle = false);

bool waitForAWhile(MIDebugSession *session, int ms, const char *file, int line);

class TestWaiter
{
public:
    TestWaiter(MIDebugSession * session_, const char * condition_, const char * file_, int line_);

    bool waitUnless(bool ok);

private:
    QTime stopWatch;
    QPointer<MIDebugSession> session;
    const char * condition;
    const char * file;
    int line;
};


} // end of namespace UnitTest
} // end of namespace KDevMI

#define WAIT_FOR_STATE(session, state) \
    do { if (!KDevMI::UnitTest::waitForState((session), (state), __FILE__, __LINE__)) return; } while (0)

#define WAIT_FOR_STATE_AND_IDLE(session, state) \
    do { if (!KDevMI::UnitTest::waitForState((session), (state), __FILE__, __LINE__, true)) return; } while (0)

#define WAIT_FOR_A_WHILE(session, ms) \
    do { if (!KDevMI::UnitTest::waitForAWhile((session), (ms), __FILE__, __LINE__)) return; } while (0)

#define WAIT_FOR(session, condition) \
    do { \
        KDevMI::UnitTest::TestWaiter w((session), #condition, __FILE__, __LINE__); \
        while (w.waitUnless((condition))) /* nothing */ ; \
    } while(0)

#define COMPARE_DATA(index, expected) \
    KDevMI::UnitTest::compareData((index), (expected), __FILE__, __LINE__)

#endif // KDEVMI_UNITTEST_H
