#include "util/CsvIO.h"

#include <QStringList>

QString csvEscape(const QString &value)
{
    if (value.contains(QLatin1Char(',')) || value.contains(QLatin1Char('"')) ||
        value.contains(QLatin1Char('\n'))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QStringLiteral("\"%1\"").arg(escaped);
    }
    return value;
}

QStringList csvParseLine(const QString &line)
{
    QStringList fields;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (inQuotes) {
            if (ch == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    current += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += ch;
            }
        } else if (ch == QLatin1Char('"')) {
            inQuotes = true;
        } else if (ch == QLatin1Char(',')) {
            fields.append(current);
            current.clear();
        } else {
            current += ch;
        }
    }

    fields.append(current);
    return fields;
}
