#pragma once

#include "model/Cuenta.h"
#include "model/ResumenMes.h"

#include <optional>
#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);

    bool open(const QString &path);
    void ensureMesActivo(const QString &mes);

    std::vector<Cuenta> cuentasDelMes(const QString &mes);

    bool crearCuenta(const QString &nombre, Moneda moneda, std::int64_t saldoInicialCentavos, const QString &mes);
    bool actualizarCuenta(std::int64_t id, const QString &nombre, std::int64_t saldoInicialCentavos,
                          std::int64_t saldoActualCentavos, const QString &mes);
    bool eliminarCuenta(std::int64_t id);

    std::optional<std::int64_t> tasaCambio(const QString &mes);
    bool guardarTasaCambio(const QString &mes, std::int64_t usdAArsCentavos);

    QStringList mesesConDatos();
    std::vector<ResumenMes> resumenHistorico();

    QString lastError() const;

private:
    bool initializeSchema();
    bool insertSaldoMes(std::int64_t cuentaId, const QString &mes, std::int64_t saldoInicial,
                        std::int64_t saldoActual);
    std::int64_t saldoActualDelMesAnterior(std::int64_t cuentaId, const QString &mes);

    QString m_connectionName;
    QString m_lastError;
};
