TEMPLATE=app
SOURCES += src/qfetcher.cpp src/main.cpp
HEADERS += src/qfetcher.h

INCLUDEPATH  = /opt/qcrawler-thirdparty/include ../qcontentcommon ../libqcontenthub
LIBS = -L /opt/qcrawler-thirdparty/lib -Wl,-rpath,../libqcontenthub -L../libqcontenthub -lqcontenthub

QT += network
