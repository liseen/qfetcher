TEMPLATE=app
SOURCES += qfetcher.cpp main.cpp
SOURCES += ../qcontentcommon/strtk.cpp\
           ../qcontentcommon/qcontent_config.cpp\

HEADERS += qfetcher.h
HEADERS += ../qcontentcommon/qcontent_config.h

INCLUDEPATH  = /opt/qcrawler-thirdparty/include ../qcontentcommon ../libqcontenthub
LIBS = -L /opt/qcrawler-thirdparty/lib -Wl,-rpath,../libqcontenthub -L../libqcontenthub -lqcontenthub -lglog

CONFIG += debug

QT += network
