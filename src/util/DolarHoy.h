#pragma once

#include <cstdint>
#include <optional>

#include <QObject>
#include <QNetworkAccessManager>

class QNetworkReply;

class DolarHoy : public QObject
{
    Q_OBJECT

public:
    explicit DolarHoy(QObject *parent = nullptr);

    void fetchDolarBlue();

    static std::optional<std::int64_t> parseDolarBluePromedio(const QByteArray &html);

signals:
    void fetched(std::int64_t promedioCentavos);
    void failed(const QString &message);

private:
    void onReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager m_network;
};