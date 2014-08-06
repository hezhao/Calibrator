#
# vim:filetype=qmake sw=4 ts=4 expandtab nospell
#

# Build configuration
# Edit this section to make sure the paths match your system configuration

# Windows 7
win32:OPENCV_DIR = "C:/opencv/build"
win32:OPENCV_LIB_DIR = $$OPENCV_DIR/x86/vc10/lib
win32:CV_VER = 249

# Debian Jessie
unix:OPENCV_DIR = "/usr/local"
unix:OPENCV_LIB_DIR = $$OPENCV_DIR/lib

# Mac OS X Mountain Lion (homebrew)
macx:OPENCV_DIR = "/usr/local"
macx:OPENCV_LIB_DIR = $$OPENCV_DIR/lib

##########################################################################

BASEDIR = ..
TOPDIR = $$BASEDIR/..
UI_DIR = GeneratedFiles
DESTDIR = $$BASEDIR/bin
FORMSDIR = $$BASEDIR/forms
SOURCEDIR = $$BASEDIR/src


NAME = scan3d-capture

CONFIG += qt
QT += opengl

CV_LIB_NAMES = core imgproc highgui calib3d features2d flann

for(lib, CV_LIB_NAMES) {
    CV_LIBS += -lopencv_$$lib
}

exists($${NAME}-custom.pri) {
    include($${NAME}-custom.pri)
}

win32 {
    DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS _USE_MATH_DEFINES
    QMAKE_CXXFLAGS_WARN_ON += -W3 -wd4396 -wd4100 -wd4996
    QMAKE_LFLAGS += /INCREMENTAL:NO
    DSHOW_LIBS = -lStrmiids -lVfw32 -lOle32 -lOleAut32

    CONFIG(release, debug|release) {
        CV_LIB_PREFIX = $$CV_VER
    }
    else {
        CV_LIB_PREFIX = $${CV_VER}d
    }
    for(lib, CV_LIBS) {
        CV_LIBS_NEW += $$lib$$CV_LIB_PREFIX
    }
    CV_LIBS = $$CV_LIBS_NEW $$CV_EXT_LIBS $$DSHOW_LIBS
}

unix:!macx {
    QMAKE_LFLAGS += -Wl,-rpath=$$OPENCV_DIR/lib
    #QMAKE_CXXFLAGS += -g
}

macx {
    LIBS += -framework Foundation -framework QTKit
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
}

CONFIG(release, debug|release) {
    TARGET = $$NAME
}
else {
    TARGET = $${NAME}_d
    CONFIG += console
}

LIBS += -L$$OPENCV_LIB_DIR $$CV_LIBS
INCLUDEPATH += $$SOURCEDIR $$UI_DIR $$OPENCV_DIR/include

include($${NAME}.pri)
