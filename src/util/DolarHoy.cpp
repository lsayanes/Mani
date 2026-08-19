#include "util/DolarHoy.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace {

std::optional<std::int64_t> parsePrecio(const QString &texto)
{
    QString s = texto.trimmed();
    s.remove(QLatin1Char('$'));
    s.remove(QLatin1Char('.'));
    s.replace(QLatin1Char(','), QLatin1Char('.'));

    bool ok = false;
    const double valor = s.toDouble(&ok);
    if (!ok || valor <= 0) {
        return std::nullopt;
    }

    return qRound64(valor * 100.0);
}

} // namespace

DolarHoy::DolarHoy(QObject *parent)
    : QObject(parent)
{
}

void DolarHoy::fetchDolarBlue()
{
    QNetworkRequest request{QUrl(QStringLiteral("https://dolarhoy.com/"))};
    request.setTransferTimeout(15000);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                                     "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Safari/605.1.15"));

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void DolarHoy::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(reply->errorString());
        return;
    }

    const QByteArray html = reply->readAll();
    const auto promedio = parseDolarBluePromedio(html);
    if (!promedio.has_value()) {
        emit failed(tr("No se encontro la cotizacion del dolar blue en dolarhoy.com"));
        return;
    }

    emit fetched(*promedio);
}

std::optional<std::int64_t> DolarHoy::parseDolarBluePromedio(const QByteArray &html)
{
    const QString page = QString::fromUtf8(html);

    static const QRegularExpression reAnchor(
        QStringLiteral(R"re(<a\b[^>]*href="([^"]*)"[^>]*>([^<]*)</a>)re"));
    static const QRegularExpression reCompra(
        QStringLiteral(R"(<div class="compra">.*?<div class="val">\$([\d.,]+)</div>)"),
        QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression reVenta(
        QStringLiteral(R"(<div class="venta">.*?<div class="val">\$([\d.,]+)</div>)"),
        QRegularExpression::DotMatchesEverythingOption);

    auto iterator = reAnchor.globalMatch(page);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch anchor = iterator.next();
        if (!anchor.captured(2).contains(QStringLiteral("blue"), Qt::CaseInsensitive)) {
            continue;
        }

        const QString window =
            page.mid(anchor.capturedEnd(), 2000);

        const QRegularExpressionMatch compraMatch = reCompra.match(window);
        const QRegularExpressionMatch ventaMatch = reVenta.match(window);
        if (!compraMatch.hasMatch() || !ventaMatch.hasMatch()) {
            continue;
        }

        const auto compra = parsePrecio(compraMatch.captured(1));
        const auto venta = parsePrecio(ventaMatch.captured(1));
        if (!compra.has_value() || !venta.has_value()) {
            continue;
        }

        return (*compra + *venta) / 2;
    }

    return std::nullopt;
}