#include "qfetcher.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = QCoreApplication::instance()->arguments();
    args.takeFirst();           // skip the first argument, which is the program's name

    qcontent::HubQueue output_queue("localhost", 9090, "crawler");
    qcontent::QFetcher fetcher(&output_queue, 1000);
    fetcher.start();

    app.exec();
}
