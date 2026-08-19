#include "util/Totales.h"

#include "model/Moneda.h"

TotalesMes calcularTotales(const std::vector<Cuenta> &cuentas)
{
    TotalesMes totales;

    for (const Cuenta &cuenta : cuentas) {
        if (cuenta.moneda == Moneda::USD) {
            totales.actualUsd += cuenta.saldoActual;
            totales.gastadoUsd += cuenta.gastado;
        } else {
            totales.actualArs += cuenta.saldoActual;
            totales.gastadoArs += cuenta.gastado;
        }
    }

    return totales;
}

std::int64_t consolidadoEnArs(std::int64_t arsCentavos, std::int64_t usdCentavos,
                              std::int64_t usdAArsCentavos)
{
    const std::int64_t product = usdCentavos * usdAArsCentavos;
    const std::int64_t usdEnArs = product >= 0 ? (product + 50) / 100 : (product - 50) / 100;
    return arsCentavos + usdEnArs;
}
