#pragma once

#include <optional>

#include <QString>

class QLabel;

QString formatMoney(std::int64_t centavos);
std::optional<std::int64_t> parseMoney(const QString &text);
void styleMoneyValue(QLabel *label, std::int64_t centavos);
