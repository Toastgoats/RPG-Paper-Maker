# See Editor.pro for explanations on Common.pri
include(../Common.pri)

# Non-gui unit tests
QT += testlib
QT -= gui

TEMPLATE = app

# testcase will cause `make check` to run the tests and output their results
CONFIG += qt console warn_on testcase
CONFIG -= app_bundle
TARGET = TestEditor

# See Editor.pro for why to use -no-pie
unix: *-g++*: QMAKE_LFLAGS += -no-pie

DESTDIR = $$ROOT_DESTDIR/Test

SOURCES += \
    test_widgettreecommands.cpp

# Use the Editor library
LIBS += -L$$MAIN_PROJECT_DIR/Build/$$VARIANT/Editor -lRPG-Paper-Maker

# Clean target explicitly (custom build paths are not included in the default clean)
# If you don't care preserving the destination folder, you can also delete DESTDIR entirely (see Editor.pro for directory removal)
win32: APP_FILENAME = $${TARGET}.exe
unix: APP_FILENAME = $$TARGET
QMAKE_CLEAN += "$$DESTDIR/$$APP_FILENAME"
