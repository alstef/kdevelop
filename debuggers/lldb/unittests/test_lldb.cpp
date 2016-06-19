/*
 * Unit tests for LLDB debugger plugin
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

#include "test_lldb.h"

#include "controllers/framestackmodel.h"
#include "debugsession.h"
#include "unittest/unittest.h"

#include <execute/iexecuteplugin.h>
#include <debugger/breakpoint/breakpointmodel.h>
#include <debugger/variable/variablecollection.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/ilaunchconfiguration.h>
#include <interfaces/iplugincontroller.h>
#include <tests/autotestshell.h>
#include <tests/testcore.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

#define SKIP_IF_ATTACH_FORBIDDEN() \
    do { \
        if (isAttachForbidden(__FILE__, __LINE__)) \
            return; \
    } while(0)

#define findSourceFile(name) \
    findSourceFile(__FILE__, (name))

using namespace KDevelop;
using namespace KDevMI::UnitTest;
using namespace KDevMI::LLDB;

namespace {
class TestLaunchConfiguration : public ILaunchConfiguration
{
public:
    TestLaunchConfiguration(const QUrl& executable = findExecutable("debugee"),
                            const QUrl& workingDirectory = QUrl()) {
        qDebug() << "FIND" << executable;
        c = new KConfig();
        c->deleteGroup("launch");
        cfg = c->group("launch");
        cfg.writeEntry("isExecutable", true);
        cfg.writeEntry("Executable", executable);
        cfg.writeEntry("Working Directory", workingDirectory);
    }
    ~TestLaunchConfiguration() override {
        delete c;
    }
    const KConfigGroup config() const override { return cfg; }
    KConfigGroup config() override { return cfg; };
    QString name() const override { return QString("Test-Launch"); }
    KDevelop::IProject* project() const override { return 0; }
    KDevelop::LaunchConfigurationType* type() const override { return 0; }
private:
    KConfigGroup cfg;
    KConfig *c;
};

class TestFrameStackModel : public LldbFrameStackModel
{
public:

    TestFrameStackModel(DebugSession* session)
        : LldbFrameStackModel(session), fetchFramesCalled(0), fetchThreadsCalled(0) {}

    void fetchFrames(int threadNumber, int from, int to) override
    {
        fetchFramesCalled++;
        LldbFrameStackModel::fetchFrames(threadNumber, from, to);
    }

    void fetchThreads() override
    {
        fetchThreadsCalled++;
        LldbFrameStackModel::fetchThreads();
    }

    int fetchFramesCalled;
    int fetchThreadsCalled;
};

class TestDebugSession : public DebugSession
{
    Q_OBJECT
public:
    TestDebugSession() : DebugSession()
    {
        setSourceInitFile(false);
        m_frameStackModel = new TestFrameStackModel(this);
        KDevelop::ICore::self()->debugController()->addSession(this);
    }

    TestFrameStackModel* frameStackModel() const override
    {
        return m_frameStackModel;
    }

private:
    TestFrameStackModel* m_frameStackModel;
};

} // end of anonymous namespace


BreakpointModel* LldbTest::breakpoints()
{
    return m_core->debugController()->breakpointModel();
}

VariableCollection *LldbTest::variableCollection()
{
    return m_core->debugController()->variableCollection();
}

Variable *LldbTest::watchVariableAt(int i)
{
    auto watchRoot = variableCollection()->indexForItem(variableCollection()->watches(), 0);
    auto idx = variableCollection()->index(i, 0, watchRoot);
    return dynamic_cast<Variable*>(variableCollection()->itemForIndex(idx));
}

Variable *LldbTest::localVariableAt(int i)
{
    auto localRoot = variableCollection()->indexForItem(variableCollection()->locals(), 0);
    auto idx = variableCollection()->index(i, 0, localRoot);
    return dynamic_cast<Variable*>(variableCollection()->itemForIndex(idx));
}

// Called before the first testfunction is executed
void LldbTest::initTestCase()
{
    AutoTestShell::init();
    m_core = TestCore::initialize(Core::NoUi);

    m_iface = ICore::self()->pluginController()
                ->pluginForExtension("org.kdevelop.IExecutePlugin", "kdevexecute")
                ->extension<IExecutePlugin>();
    Q_ASSERT(m_iface);

    m_debugeeFileName = findSourceFile("debugee.cpp");
}

// Called after the last testfunction was executed
void LldbTest::cleanupTestCase()
{
    TestCore::shutdown();
}

// Called before each testfunction is executed
void LldbTest::init()
{
    //remove all breakpoints - so we can set our own in the test
    KConfigGroup bpCfg = KSharedConfig::openConfig()->group("breakpoints");
    bpCfg.writeEntry("number", 0);
    bpCfg.sync();

    breakpoints()->removeRows(0, breakpoints()->rowCount());

    while (variableCollection()->watches()->childCount() > 0) {
        auto var = watchVariableAt(0);
        if (!var) break;
        var->die();
    }
}

void LldbTest::cleanup()
{
    // Called after every testfunction
}

void LldbTest::testStdout()
{
    TestDebugSession *session = new TestDebugSession;

    QSignalSpy outputSpy(session, &TestDebugSession::inferiorStdoutLines);

    TestLaunchConfiguration cfg;
    session->startDebugging(&cfg, m_iface);
    WAIT_FOR_STATE(session, KDevelop::IDebugSession::EndedState);

    QCOMPARE(outputSpy.count(), 2);
    QList<QVariant> arguments = outputSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.first().toStringList(), QStringList() << "Hello, world!");

    arguments = outputSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.first().toStringList(), QStringList() << "Hello");
}

void LldbTest::testVariablesLocals()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(IVariableController::UpdateLocals);

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 22);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(1000);

    QCOMPARE(variableCollection()->rowCount(), 2);
    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "i");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "0");
    session->run();
    QTest::qWait(1000);
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "i");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");
    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesLocalsStruct()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(1000);

    QModelIndex i = variableCollection()->index(1, 0);
    QCOMPARE(variableCollection()->rowCount(i), 4);

    int structIndex = 0;
    for(int j=0; j<3; ++j) {
        if (variableCollection()->index(j, 0, i).data().toString() == "ts") {
            structIndex = j;
        }
    }

    COMPARE_DATA(variableCollection()->index(structIndex, 0, i), "ts");
    COMPARE_DATA(variableCollection()->index(structIndex, 1, i), "{...}");
    QModelIndex ts = variableCollection()->index(structIndex, 0, i);
    COMPARE_DATA(variableCollection()->index(0, 0, ts), "...");
    variableCollection()->expanded(ts);
    QTest::qWait(100);
    COMPARE_DATA(variableCollection()->index(0, 0, ts), "a");
    COMPARE_DATA(variableCollection()->index(0, 1, ts), "0");
    COMPARE_DATA(variableCollection()->index(1, 0, ts), "b");
    COMPARE_DATA(variableCollection()->index(1, 1, ts), "1");
    COMPARE_DATA(variableCollection()->index(2, 0, ts), "c");
    COMPARE_DATA(variableCollection()->index(2, 1, ts), "2");

    session->stepInto();
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(1000);
    COMPARE_DATA(variableCollection()->index(structIndex, 0, i), "ts");
    COMPARE_DATA(variableCollection()->index(structIndex, 1, i), "{...}");
    COMPARE_DATA(variableCollection()->index(0, 1, ts), "1");

    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesWatches()
{
    TestDebugSession *session = new TestDebugSession;
    KDevelop::ICore::self()->debugController()->variableCollection()->variableWidgetShown();

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    variableCollection()->watches()->add("ts");
    QTest::qWait(300);

    QModelIndex i = variableCollection()->index(0, 0);
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "ts");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "{...}");
    QModelIndex ts = variableCollection()->index(0, 0, i);
    COMPARE_DATA(variableCollection()->index(0, 0, ts), "...");
    variableCollection()->expanded(ts);
    QTest::qWait(100);
    COMPARE_DATA(variableCollection()->index(0, 0, ts), "a");
    COMPARE_DATA(variableCollection()->index(0, 1, ts), "0");
    COMPARE_DATA(variableCollection()->index(1, 0, ts), "b");
    COMPARE_DATA(variableCollection()->index(1, 1, ts), "1");
    COMPARE_DATA(variableCollection()->index(2, 0, ts), "c");
    COMPARE_DATA(variableCollection()->index(2, 1, ts), "2");

    session->stepInto();
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(100);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "ts");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "{...}");
    COMPARE_DATA(variableCollection()->index(0, 1, ts), "1");

    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesWatchesQuotes()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateWatches);

    TestLaunchConfiguration cfg;

    const QString testString("test");
    const QString quotedTestString("\"" + testString + "\"");

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    variableCollection()->watches()->add(quotedTestString); //just a constant string
    QTest::qWait(300);

    QModelIndex i = variableCollection()->index(0, 0);
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), quotedTestString);
    COMPARE_DATA(variableCollection()->index(0, 1, i), "[" + QString::number(testString.length() + 1) + "]");

    QModelIndex testStr = variableCollection()->index(0, 0, i);
    COMPARE_DATA(variableCollection()->index(0, 0, testStr), "...");
    variableCollection()->expanded(testStr);
    QTest::qWait(100);
    int len = testString.length();
    for (int ind = 0; ind < len; ind++)
    {
        COMPARE_DATA(variableCollection()->index(ind, 0, testStr), QString::number(ind));
        QChar c = testString.at(ind);
        QString value = QString::number(c.toLatin1()) + " '" + c + "'";
        COMPARE_DATA(variableCollection()->index(ind, 1, testStr), value);
    }
    COMPARE_DATA(variableCollection()->index(len, 0, testStr), QString::number(len));
    COMPARE_DATA(variableCollection()->index(len, 1, testStr), "0 '\\000'");

    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesWatchesTwoSessions()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateWatches);

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    variableCollection()->watches()->add("ts");
    QTest::qWait(300);

    QModelIndex ts = variableCollection()->index(0, 0, variableCollection()->index(0, 0));
    variableCollection()->expanded(ts);
    QTest::qWait(100);
    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);

    //check if variable is marked as out-of-scope
    QCOMPARE(variableCollection()->watches()->childCount(), 1);
    KDevelop::Variable* v = watchVariableAt(0);
    QVERIFY(v);
    QVERIFY(!v->inScope());

    // TODO: not sure what this means, reenable this after fix access problem
    /*
    QCOMPARE(v->childCount(), 3);
    v = dynamic_cast<KDevelop::Variable*>(v->child(0));
    QVERIFY(!v->inScope());
    */

    //start a second debug session
    session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateWatches);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(300);

    QCOMPARE(variableCollection()->watches()->childCount(), 1);
    ts = variableCollection()->index(0, 0, variableCollection()->index(0, 0));
    v = watchVariableAt(0);
    QVERIFY(v);
    QVERIFY(v->inScope());

    // TODO: not sure what this means, reenable this after fix access problem
    /*
    QCOMPARE(v->childCount(), 3);
    v = dynamic_cast<KDevelop::Variable*>(v->child(0));
    QVERIFY(v->inScope());
    QCOMPARE(v->data(1, Qt::DisplayRole).toString(), QString::number(0));
    */

    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);

    //check if variable is marked as out-of-scope
    // TODO: not sure what this means, reenable this after fix access problem
    /*
    v = dynamic_cast<KDevelop::Variable*>(variableCollection()->watches()->child(0));
    QVERIFY(!v->inScope());
    QVERIFY(!dynamic_cast<KDevelop::Variable*>(v->child(0))->inScope());
    */
}

void LldbTest::testVariablesStopDebugger()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    session->stopDebugger();
    QTest::qWait(300);
}


void LldbTest::testVariablesStartSecondSession()
{
    TestDebugSession *session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);

    TestLaunchConfiguration cfg;

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    session = new TestDebugSession;
    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 38);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);

    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesSwitchFrame()
{
    TestDebugSession *session = new TestDebugSession;
    TestLaunchConfiguration cfg;

    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    TestFrameStackModel *stackModel = session->frameStackModel();

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 24);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(500);

    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "i");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");

    stackModel->setCurrentFrame(1);
    QTest::qWait(200);

    i = variableCollection()->index(1, 0);
    QCOMPARE(variableCollection()->rowCount(i), 4);
    COMPARE_DATA(variableCollection()->index(2, 0, i), "argc");
    COMPARE_DATA(variableCollection()->index(2, 1, i), "1");
    COMPARE_DATA(variableCollection()->index(3, 0, i), "argv");

    breakpoints()->removeRow(0);
    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

void LldbTest::testVariablesQuicklySwitchFrame()
{
    TestDebugSession *session = new TestDebugSession;
    TestLaunchConfiguration cfg;

    session->variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    TestFrameStackModel *stackModel = session->frameStackModel();

    breakpoints()->addCodeBreakpoint(QUrl::fromLocalFile(m_debugeeFileName), 24);
    QVERIFY(session->startDebugging(&cfg, m_iface));
    WAIT_FOR_STATE(session, DebugSession::PausedState);
    QTest::qWait(500);

    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "i");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");

    stackModel->setCurrentFrame(1);
    QTest::qWait(300);
    stackModel->setCurrentFrame(0);
    QTest::qWait(1);
    stackModel->setCurrentFrame(1);
    QTest::qWait(1);
    stackModel->setCurrentFrame(0);
    QTest::qWait(1);
    stackModel->setCurrentFrame(1);
    QTest::qWait(500);

    i = variableCollection()->index(1, 0);
    QCOMPARE(variableCollection()->rowCount(i), 4);
    QStringList locs;
    for (int j = 0; j < variableCollection()->rowCount(i); ++j) {
        locs << variableCollection()->index(j, 0, i).data().toString();
    }
    QVERIFY(locs.contains("argc"));
    QVERIFY(locs.contains("argv"));
    QVERIFY(locs.contains("x"));

    breakpoints()->removeRow(0);
    session->run();
    WAIT_FOR_STATE(session, DebugSession::EndedState);
}

QTEST_MAIN(KDevMI::LLDB::LldbTest);

#include "test_lldb.moc"
