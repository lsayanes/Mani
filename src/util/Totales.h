#pragma once

#include "model/Cuenta.h"

#include <cstdint>
#include <vector>

struct TotalesMes
{
    std::int64_t actualUsd = 0;
    std::int64_t actualArs = 0;
    std::int64_t gastadoUsd = 0;
    std::int64_t gastadoArs = 0;
};

TotalesMes calcularTotales(const std::vector<Cuenta> &cuentas);
std::int64_t consolidadoEnArs(std::int64_t arsCentavos, std::int64_t usdCentavos,
                              std::int64_t usdAArsCentavos);
