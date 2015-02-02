#-------------------------------------------------
#
# Project created by QtCreator 2012-03-22T19:28:15
#
#-------------------------------------------------

QT       += core gui webkit network xml

TARGET = vkmm
TEMPLATE = app

INCLUDEPATH += src

# Include Qt-Single-App:
include (3rdpatry/qtsingleapplication/src/qtsingleapplication.pri)

# Include Qt-Json:
INCLUDEPATH += 3rdpatry/qjson/include

SOURCES += src/main.cpp \
    src/core/core.cpp \
    src/ui/wndmain.cpp \
    src/ui/wndloginbrowser.cpp \
    src/core/mnetworkcookiejar.cpp \
    src/core/mnetowrkaccess.cpp \
    src/ui/wndsessionload.cpp \
    src/core/mapivk.cpp \
    src/core/mplaylist.cpp \
    src/core/mrefcounter.cpp \
    src/ui/wdgplaylist.cpp \
    src/core/mplayer.cpp \
    src/core/mcacheaudio.cpp \
    src/ui/wdgplayslider.cpp \
    src/core/common.cpp \
    src/ui/wndsendvkfirendaudio.cpp \
    src/ui/wndselectvkfriends.cpp \
    src/core/mapilastfm.cpp \
    src/ui/wndsettings.cpp \
    src/core/mstream.cpp \
    src/ui/wndabout.cpp \
    src/core/mupdater.cpp \
    src/ui/wndupdate.cpp \
    src/ui/wndupdatewin.cpp \
    src/ui/wndmessage.cpp \
    src/ui/wdgweb.cpp \
    src/ui/wdglfmlinecompleted.cpp

HEADERS  += \ 
    src/core/core.h \
    src/ui/wndmain.h \
    src/ui/wndloginbrowser.h \
    src/config.h \
    src/core/mnetworkcookiejar.h \
    src/core/mnetowrkaccess.h \
    src/ui/wndsessionload.h \
    src/core/mapivk.h \
    src/core/mplaylist.h \
    src/core/mrefcounter.h \
    src/ui/wdgplaylist.h \
    src/core/mplayer.h \
    src/core/mcacheaudio.h \
    src/ui/wdgplayslider.h \
    src/core/common.h \
    src/ui/wndsendvkfirendaudio.h \
    src/ui/wndselectvkfriends.h \
    src/core/mapilastfm.h \
    src/ui/wndsettings.h \
    src/core/mstream.h \
    src/version.h \
    src/ui/wndabout.h \
    src/core/mupdater.h \
    src/ui/wndupdate.h \
    src/ui/wndupdatewin.h \
    src/ui/wndmessage.h \
    src/ui/wdgweb.h \
    src/ui/wdglfmlinecompleted.h

FORMS    += \ 
    src/ui/wndmain.ui \
    src/ui/wndloginbrowser.ui \
    src/ui/wndsessionload.ui \
    src/ui/wndsendvkfirendaudio.ui \
    src/ui/wndselectvkfriends.ui \
    src/ui/wndsettings.ui \
    src/ui/wndabout.ui \
    src/ui/wndupdate.ui \
    src/ui/wndupdatewin.ui \
    src/ui/wndmessage.ui

# BASS lib include
win32:{
    INCLUDEPATH += ./bass/
    LIBS += ./bass/lib/bass.lib
}
unix:{
    INCLUDEPATH += ./bass/
    LIBS += -L./ -L./bass/lib -L./bass/lib-linux-32 -L./bass/lib-linux-64 -lbass -Wl,-rpath,./
    LIBS += -ldl
}

RESOURCES += \
    res/res.qrc

OTHER_FILES += \
    res/css/global-win.css \
    res/css/global-linux.css \
    res/res.rc \
    res/sys/vkmm-msdos-update.bat

RC_FILE = res/res.rc
