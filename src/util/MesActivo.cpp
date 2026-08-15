#include "util/MesActivo.h"

#include <QDate>

QString mesActivoActual()
{
    return QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
}

QString mesAnterior(const QString &mes)
{
    const QDate date = QDate::fromString(mes + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return {};
    }

    return date.addMonths(-1).toString(QStringLiteral("yyyy-MM"));
}
