#include <stdio.h>
#include <stdlib.h>
#include <glog/logging.h>

#include <QDebug>
#include <QUrl>
#include <QDateTime>
#include <QTime>
#include <QCoreApplication>
#include <cassert>
#include <string>
#include <msgpack.hpp>

#include "qfetcher.h"

namespace qcontent {

void QFetcher::cycleFetch()
{
    VLOG(1) << __FUNCTION__;
    if (m_global_stop) {
        return;
    }

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
    VLOG(1) << __FUNCTION__;
    QList<std::string>  tmp;
    int size = m_record_buffer.size();
    for (int i = 0; i < size; i++) {
        int push_ret = pushCrawlerRecord(m_record_buffer[i]);
        if (push_ret == QCONTENTHUB_OK) {
        } else if (push_ret == QCONTENTHUB_AGAIN) {
            tmp.push_back(m_record_buffer[i]);
        } else if (push_ret == QCONTENTHUB_WARN) {
            LOG(WARNING) << "push crawled record warn";
        } else if (push_ret == QCONTENTHUB_ERROR) {
            LOG(ERROR) << "push crawled record error";
        }
    }
    m_record_buffer.clear();
    m_record_buffer.append(tmp);
}


void QFetcher::crawlUrl(const QUrl &qurl, QCrawlerRecord &record)
{
    VLOG(1) << "crawl: " << record.url;
    QNetworkRequest request(qurl);
    QNetworkReply *reply = m_manager.get(request);

    record.download_time = QDateTime::currentDateTime().toTime_t();
    m_current_downloads.insert(reply, record);
}

void QFetcher::start()
{
    m_global_stop = false;

    if (fetch_timer == NULL) {
        fetch_timer = new QTimer(this);
        connect(fetch_timer, SIGNAL(timeout()), this, SLOT(cycleFetch()));
    }
    if (push_timer == NULL) {
        push_timer = new QTimer(this);
        connect(push_timer, SIGNAL(timeout()), this, SLOT(cyclePush()));
    }


    fetch_timer->start(1000);
    push_timer->start(1000);
}

void QFetcher::cycleStop()
{
    VLOG(1) << __FUNCTION__;
    if (m_global_stop && m_current_downloads.size() == 0 &&
            m_record_buffer.size() == 0) {
        VLOG(1) << "quit application";
        QCoreApplication::quit();
    }
}

void QFetcher::stop()
{
    VLOG(1) << __FUNCTION__;
    m_global_stop = true;
    if (stop_timer == NULL) {
        stop_timer = new QTimer(this);
        connect(stop_timer, SIGNAL(timeout()), this, SLOT(cycleStop()));
        stop_timer->start(1000);
    }
}

int QFetcher::fetchCrawlerRecord()
{
    QCrawlerRecord record;
    std::string record_str;
    int ret = m_input_queue->pop_url(record_str);
    VLOG(1) << "pop_url ret: " << ret;
    if (ret == QCONTENTHUB_OK) {
        msgpack::zone zone;
        msgpack::object obj;
        try {
            msgpack::unpack(record_str.c_str(), record_str.size(), NULL, &zone, &obj);
            obj.convert(&record);
        } catch (std::exception& e) {
            // TODO
            LOG(ERROR) << "uppack crawler record error " << e.what();
            return QCONTENTHUB_ERROR;
        }
        QUrl qurl(QString::fromUtf8(record.url.c_str()));
        crawlUrl(qurl, record);
    }

    return ret;
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
    VLOG(1) << __FUNCTION__;
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
        rec.crawled_okay = false;
        int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        rec.crawl_failed_times++;
        rec.http_code = http_code;
        pushCrawlerRecord(rec);
       } else {
        int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http_code == 301  || http_code == 302) {
            rec.is_redirect = true;
            if (rec.redirect_times > 4) {
                rec.crawled_okay = false;
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
            rec.crawled_okay = true;
            //rec.crawl_failed_time = 0
            QByteArray data = reply->readAll();

            rec.raw_html_content = std::string(data.data(), data.size());
            pushCrawlerRecord(rec);
        } else  {

        }
    }

    VLOG(1) << "download: " << rec.url << " okay";
    m_current_downloads.remove(reply);
    reply->deleteLater();
}

} //end namespace qcrawler
