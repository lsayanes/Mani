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

QString mesSiguiente(const QString &mes)
{
    const QDate date = QDate::fromString(mes + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return {};
    }

    return date.addMonths(1).toString(QStringLiteral("yyyy-MM"));
}

bool esMesValido(const QString &mes)
{
    return QDate::fromString(mes + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd")).isValid();
}

int compararMeses(const QString &a, const QString &b)
{
    const QDate dateA = QDate::fromString(a + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"));
    const QDate dateB = QDate::fromString(b + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"));
    if (!dateA.isValid() || !dateB.isValid()) {
        return 0;
    }
    if (dateA < dateB) {
        return -1;
    }
    if (dateA > dateB) {
        return 1;
    }
    return 0;
}
