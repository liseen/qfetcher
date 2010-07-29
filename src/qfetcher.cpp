#include "qfetcher.h"
#include <stdio.h>
#include <stdlib.h>

#include <QDebug>
#include <QUrl>
#include <QDateTime>
#include <QTime>
#include <cassert>

namespace qcontent {

void QFetcher::cycleFetch()
{
    if (m_stop_crawl || m_record_buffer.size() >= m_concurrent) {
        return;
    }

    while (m_current_downloads.size() < m_concurrent) {
        int fetch_ret = fetchCrawlerRecord();
        if (fetch_ret != QCONTENTHUB_OK) {
            break;
        }
    }
}

void QFetcher::cyclePush()
{
    QList<std::string>  tmp;
    int size = m_record_buffer.size();
    for (int i = 0; i < size; i++) {
        int push_ret = pushCrawlerRecord(m_record_buffer[i]);
        if (push_ret == QCONTENTHUB_OK) {
        } else {
            tmp.push_back(m_record_buffer[i]);
        }
    }
    m_record_buffer.clear();
    m_record_buffer.append(tmp);
}


void QFetcher::crawlUrl(const QUrl &qurl, QCrawlerRecord &record)
{
    qDebug() << "crawl: " << qurl.toString();
    QNetworkRequest request(qurl);
    QNetworkReply *reply = m_manager.get(request);

    record.download_time = QDateTime::currentDateTime().toTime_t();
    m_current_downloads.insert(reply, record);
}

void QFetcher::start()
{
    if (fetch_timer == NULL) {
        fetch_timer = new QTimer(this);
    }
    if (push_timer == NULL) {
        push_timer = new QTimer(this);
    }

    connect(fetch_timer, SIGNAL(timeout()), this, SLOT(cycleFetch()));
    connect(push_timer, SIGNAL(timeout()), this, SLOT(cyclePush()));

    fetch_timer->start(1000);
    push_timer->start(1000);
}

int QFetcher::fetchCrawlerRecord()
{
    QCrawlerRecord record;
    record.url = "http://www.cnblogs.com/kaima/archive/2009/08/25/1553510.html";
    record.url = "http://localhost/";
    QUrl qurl(QString::fromUtf8(record.url.c_str()));
    crawlUrl(qurl, record);
     // TODO fetch and begin download
     //
     return 0;
}

int QFetcher::pushCrawlerRecord(const QCrawlerRecord &record)
{
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, record);
    std::string rec_str(sbuf.data(), sbuf.size());
    return pushCrawlerRecord(rec_str);
}

int QFetcher::pushCrawlerRecord(const std::string &rec_str)
{
    int push_ret = m_out_queue->push_nowait(rec_str);
    if (push_ret == QCONTENTHUB_OK) {
        m_stop_crawl = false;
        fetchCrawlerRecord();
    } else {
        m_stop_crawl = true;
        m_record_buffer.push_back(rec_str);
    }

    return push_ret;
}


void QFetcher::downloadFinished(QNetworkReply *reply)
{
    QCrawlerRecord &rec = m_current_downloads[reply];

    if (reply->error()) {
        rec.crawl_okay = false;
        int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        rec.crawl_failed_times++;
        rec.http_code = http_code;
        pushCrawlerRecord(rec);
       } else {
        int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http_code == 301  || http_code == 302) {
            rec.is_redirect = true;
            if (rec.redirect_times > 4) {
                rec.crawl_okay = false;
                pushCrawlerRecord(rec);
            } else {
                // continue crawl redict url
                rec.redirect_times++;
                QUrl qredirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                if (qredirect.isRelative()) {
                    qredirect = reply->url().resolved(qredirect);
                }

                rec.redirect_url = std::string(qredirect.toString().toUtf8().constData());

                crawlUrl(qredirect, rec);
            }
        } else if (http_code == 200) {
            rec.crawl_okay = true;
            //rec.crawl_failed_time = 0
            QByteArray data = reply->readAll();

            rec.raw_html_content = std::string(data.data(), data.size());
            pushCrawlerRecord(rec);
        } else  {

        }
    }

    qDebug() << "download: " << reply->url() << " okay";
/*
 * QTime
    frame->setContent(reply->readAll(), "text/html", url);
    printf("time elapsed: %d\n", begin_time.elapsed());
*/
    m_current_downloads.remove(reply);
    reply->deleteLater();
}

} //end namespace qcrawler
