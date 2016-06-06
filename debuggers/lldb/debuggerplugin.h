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

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include <QByteArray>
#include <QLabel>
#include <QVariant>

#include <KConfigGroup>
#include <KTextEditor/Cursor>
#include <QUrl>

#include <interfaces/iplugin.h>
#include <interfaces/istatus.h>

class QLabel;
class QMenu;
class QDBusInterface;
class QSignalMapper;
class ProcessWidget;

class KToolBar;
class QAction;

namespace KDevelop {
class Context;
class ProcessLineMaker;
}

namespace KDevMI {
namespace LLDB
{
class DisassembleWidget;
class Breakpoint;
class LLDBOutputWidget;
class MemoryViewerWidget;
class DebugSession;
template<typename T> class DebuggerToolFactory;

class CppDebuggerPlugin : public KDevelop::IPlugin, public KDevelop::IStatus
{
    Q_OBJECT
    Q_INTERFACES(KDevelop::IStatus)

public:
    friend class DebugSession;

    CppDebuggerPlugin( QObject *parent, const QVariantList & = QVariantList() );
    ~CppDebuggerPlugin() override;

    void unload() override;
    
    KDevelop::ContextMenuExtension contextMenuExtension( KDevelop::Context* ) override;
    
    DebugSession *createSession();

public:
    //BEGIN IStatus
    QString statusName() const override;

Q_SIGNALS:
    void clearMessage(KDevelop::IStatus*) override;
    void showMessage(KDevelop::IStatus*, const QString & message, int timeout = 0) override;
    void hideProgress(KDevelop::IStatus*) override;
    void showProgress(KDevelop::IStatus*, int minimum, int maximum, int value) override;
    void showErrorMessage(const QString&, int) override;
    //END IStatus

    void addWatchVariable(const QString& variable);
    void evaluateExpression(const QString& variable);

    void raiseLldbConsoleViews();

    void reset();

private Q_SLOTS:
    void setupDBus();
    void slotDebugExternalProcess(QObject* interface);
    void contextEvaluate();
    void contextWatch();

    void slotExamineCore();
    #ifdef KDEV_ENABLE_LLDB_ATTACH_DIALOG
    void slotAttachProcess();
    #endif

    void slotDBusServiceRegistered(const QString& service);
    void slotDBusServiceUnregistered(const QString& service);

    void slotCloseDrKonqi();

    void slotFinished();

    void controllerMessage(const QString&, int);

Q_SIGNALS:
    //TODO: port to launch framework
    //void startDebugger(const KDevelop::IRun & run, KJob* job);
    void stopDebugger();
    void attachTo(int pid);
    void coreFile(const QString& core);
    void runUntil(const QUrl &url, int line);
    void jumpTo(const QUrl &url, int line);

protected:
    void initializeGuiState() override;

private:
    KConfigGroup config() const;

    void attachProcess(int pid);
    void setupActions();

    QHash<QString, QDBusInterface*> m_drkonqis;
    QSignalMapper* m_drkonqiMap;
    QString m_drkonqi;

    QString m_contextIdent;

    KConfigGroup m_config;
};

} // end of namespace LLDB
} // end of namespace KDevMI

#endif // DEBUGGERPLUGIN_H
