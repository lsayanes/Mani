#pragma once

#include <cstdint>
#include <optional>

#include <QString>

struct ResumenMes
{
    QString mes;
    std::int64_t gastadoUsd = 0;
    std::int64_t gastadoArs = 0;
    std::optional<std::int64_t> tasa;
};
