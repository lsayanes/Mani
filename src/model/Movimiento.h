#pragma once

#include "model/Moneda.h"

#include <cstdint>

#include <QDate>
#include <QString>

struct Movimiento
{
    std::int64_t id = 0;
    std::int64_t cuentaId = 0;
    QString mes;
    QDate fecha;
    std::int64_t monto = 0;
    QString concepto;
    QString categoria;
    Moneda moneda = Moneda::ARS;
};
