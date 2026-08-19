#pragma once

#include "model/Cuenta.h"
#include "model/GastoCategoria.h"
#include "model/Movimiento.h"
#include "model/ResumenMes.h"

#include <optional>
#include <vector>

#include <QDate>
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
                          const QString &mes);
    bool eliminarCuenta(std::int64_t id);

    bool crearMovimiento(std::int64_t cuentaId, const QDate &fecha, std::int64_t montoCentavos,
                         const QString &concepto, const QString &categoria = {});
    bool actualizarMovimiento(std::int64_t movimientoId, const QDate &fecha,
                              std::int64_t montoCentavos, const QString &concepto,
                              const QString &categoria = {});
    bool eliminarMovimiento(std::int64_t movimientoId);
    std::vector<Movimiento> movimientosDeCuenta(std::int64_t cuentaId, const QString &mes);

    QStringList categoriasConocidas();
    std::vector<GastoCategoria> gastoPorCategoria(const QString &mes, Moneda moneda);

    std::optional<std::int64_t> tasaCambio(const QString &mes);
    bool guardarTasaCambio(const QString &mes, std::int64_t usdAArsCentavos);

    QStringList mesesConDatos();
    std::vector<ResumenMes> resumenHistorico();

    bool exportCsv(const QString &directoryPath);
    bool importCsv(const QString &directoryPath);
    bool backupToFile(const QString &destinationPath);
    bool restoreFromFile(const QString &sourcePath);

    void close();
    QString dbPath() const;
    bool isOpen() const;

    QString lastError() const;

private:
    bool initializeSchema();
    bool migrateSchema();
    bool insertSaldoMes(std::int64_t cuentaId, const QString &mes, std::int64_t saldoInicial,
                        std::int64_t saldoActual);
    bool ensureSaldoMesForCuenta(std::int64_t cuentaId, const QString &mes);
    bool aplicarMontoSaldo(std::int64_t cuentaId, const QString &mes, std::int64_t deltaCentavos);
    std::int64_t saldoActualDelMesAnterior(std::int64_t cuentaId, const QString &mes);
    bool reopen();

    QString m_connectionName;
    QString m_dbPath;
    QString m_lastError;
};
