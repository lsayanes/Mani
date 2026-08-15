#pragma once

#include "model/Moneda.h"

#include <cstdint>

#include <QString>

struct Cuenta
{
    std::int64_t id = 0;
    QString nombre;
    Moneda moneda = Moneda::ARS;
    std::int64_t saldoInicial = 0;
    std::int64_t saldoActual = 0;

    std::int64_t gastado() const
    {
        return saldoInicial - saldoActual;
    }
};
