#include "util/Money.h"

#include <QLabel>
#include <QLocale>

namespace {

QLocale appLocale()
{
    return QLocale(QStringLiteral("es_AR"));
}

} // namespace

QString formatMoney(std::int64_t centavos)
{
    const bool negative = centavos < 0;
    const std::int64_t absolute = negative ? -centavos : centavos;
    const std::int64_t whole = absolute / 100;
    const std::int64_t fraction = absolute % 100;

    QString formatted = appLocale().toString(static_cast<qlonglong>(whole));
    formatted += QLocale(QStringLiteral("es_AR")).decimalPoint();
    formatted += QStringLiteral("%1").arg(fraction, 2, 10, QChar('0'));

    if (negative) {
        formatted.prepend(QStringLiteral("-"));
    }

    return QStringLiteral("$ %1").arg(formatted);
}

std::optional<std::int64_t> parseMoney(const QString &text)
{
    QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        return std::nullopt;
    }

    normalized.remove(QChar('$'));
    normalized = normalized.trimmed();

    const QLocale locale(QStringLiteral("es_AR"));
    bool ok = false;
    const double value = locale.toDouble(normalized, &ok);
    if (!ok) {
        return std::nullopt;
    }

    const long long centavos = qRound64(value * 100.0);
    return static_cast<std::int64_t>(centavos);
}

void styleMoneyValue(QLabel *label, std::int64_t centavos)
{
    label->setText(formatMoney(centavos));
    if (centavos < 0) {
        label->setStyleSheet(QStringLiteral("color: #c0392b;"));
    } else if (centavos > 0) {
        label->setStyleSheet(QStringLiteral("color: #27ae60;"));
    } else {
        label->setStyleSheet({});
    }
}
