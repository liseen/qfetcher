#ifndef QCONTENT_FETCHER_H
#define QCONTENT_FETCHER_H

#include <QCoreApplication>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QHash>
#include <string>

#include <libqcontentqueue.h>
#include <qcontent_record.h>

namespace qcontent {

class QFetcher: public QObject
{
    Q_OBJECT
public:
    QFetcher(HubQueue *out_queue, int concurrent = 10) : m_out_queue(out_queue), m_concurrent(concurrent), m_stop_crawl(false) {
        fetch_timer = NULL;
        push_timer = NULL;
        connect(&m_manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(downloadFinished(QNetworkReply*)));
    }

    void start();
    void crawlUrl(const QUrl &qurl, QCrawlerRecord &record);
    int fetchCrawlerRecord();
    int pushCrawlerRecord(const QCrawlerRecord &record);
    int pushCrawlerRecord(const std::string &record);

public slots:
    void cycleFetch();
    void cyclePush();
    void downloadFinished(QNetworkReply *reply);

private:
    HubQueue *m_out_queue;
    QNetworkAccessManager m_manager;
    QHash<QNetworkReply *, QCrawlerRecord> m_current_downloads;
    QList<std::string> m_record_buffer;
    QTimer *fetch_timer;
    QTimer *push_timer;


    int m_concurrent;
    volatile bool m_stop_crawl;
};

} // end namespace qcrawler

#endif
