#pragma once

#include <cstdint>

#include <QString>

struct GastoCategoria
{
    QString categoria;
    std::int64_t totalCentavos = 0;
};
