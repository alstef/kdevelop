project(lldb)
add_definitions(-DTRANSLATION_DOMAIN=\"kdevlldb\")

set(kdevlldb_SRCS
    lldbdebugger.cpp
    lldbcommand.cpp
    debugsession.cpp
    controllers/breakpointcontroller.cpp
    controllers/variablecontroller.cpp
    controllers/variable.cpp
    controllers/framestackmodel.cpp
)

set(kdevlldb_UI
)

ki18n_wrap_ui(kdevlldb_SRCS ${kdevlldb_UI})
qt5_add_resources(kdevlldb_SRCS kdevlldb.qrc)

# common code used by plugin and unit test
add_library(kdevlldb_static STATIC ${kdevlldb_SRCS})
target_link_libraries(kdevlldb_static
    PUBLIC
    kdevdebuggercommon
    KDev::Debugger
    PRIVATE
    Qt5::Core
)

# The actual plugin
kdevplatform_add_plugin(kdevlldb JSON kdevlldb.json SOURCES debuggerplugin.cpp)
target_link_libraries(kdevlldb
    PUBLIC
    kdevlldb_static
    KDev::Interfaces
    KDev::Language
    KDev::Debugger
    KDev::OutputView
    KDev::Project
    KDev::Util
    KF5::TextEditor
    PRIVATE
    KDev::Sublime
)

# Unit tests
set(test_lldb_SRCS unittests/test_lldb.cpp)
ecm_add_test(${test_lldb_SRCS}
    TEST_NAME test_lldb
    LINK_LIBRARIES
        kdevdebugger_unittest
        kdevlldb_static
        Qt5::Test
        KDev::Tests
)
