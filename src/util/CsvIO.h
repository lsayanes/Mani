#pragma once

#include <QString>

QString csvEscape(const QString &value);
QStringList csvParseLine(const QString &line);
