#include "database/Database.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {

Moneda monedaFromInt(int value)
{
    return static_cast<Moneda>(value);
}

} // namespace

Database::Database(QObject *parent)
    : QObject(parent)
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

bool Database::open(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.dir().exists() && !QDir().mkpath(fileInfo.absolutePath())) {
        m_lastError = QStringLiteral("No se pudo crear el directorio de datos.");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery foreignKeys(db);
    if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        m_lastError = foreignKeys.lastError().text();
        return false;
    }

    return initializeSchema() && migrateSchema();
}

void Database::ensureMesActivo(const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id FROM cuenta ORDER BY id"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return;
    }

    while (query.next()) {
        const std::int64_t cuentaId = query.value(0).toLongLong();

        QSqlQuery existsQuery(db);
        existsQuery.prepare(QStringLiteral(
            "SELECT 1 FROM saldo_mes WHERE cuenta_id = :cuenta_id AND mes = :mes"));
        existsQuery.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
        existsQuery.bindValue(QStringLiteral(":mes"), mes);
        if (!existsQuery.exec()) {
            m_lastError = existsQuery.lastError().text();
            return;
        }

        if (existsQuery.next()) {
            continue;
        }

        const std::int64_t saldoAnterior = saldoActualDelMesAnterior(cuentaId, mes);
        if (saldoAnterior < 0) {
            continue;
        }

        if (!insertSaldoMes(cuentaId, mes, saldoAnterior, saldoAnterior)) {
            return;
        }
    }
}

std::vector<Cuenta> Database::cuentasDelMes(const QString &mes)
{
    std::vector<Cuenta> cuentas;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT c.id, c.nombre, c.moneda, s.saldo_inicial, s.saldo_actual "
        "FROM cuenta c "
        "INNER JOIN saldo_mes s ON s.cuenta_id = c.id "
        "WHERE s.mes = :mes "
        "ORDER BY c.id"));
    query.bindValue(QStringLiteral(":mes"), mes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return cuentas;
    }

    while (query.next()) {
        Cuenta cuenta;
        cuenta.id = query.value(0).toLongLong();
        cuenta.nombre = query.value(1).toString();
        cuenta.moneda = monedaFromInt(query.value(2).toInt());
        cuenta.saldoInicial = query.value(3).toLongLong();
        cuenta.saldoActual = query.value(4).toLongLong();
        cuentas.push_back(cuenta);
    }

    return cuentas;
}

bool Database::crearCuenta(const QString &nombre, Moneda moneda, std::int64_t saldoInicialCentavos,
                           const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery insertCuenta(db);
    insertCuenta.prepare(QStringLiteral(
        "INSERT INTO cuenta (nombre, moneda, creado_en) VALUES (:nombre, :moneda, :creado_en)"));
    insertCuenta.bindValue(QStringLiteral(":nombre"), nombre);
    insertCuenta.bindValue(QStringLiteral(":moneda"), static_cast<int>(moneda));
    insertCuenta.bindValue(QStringLiteral(":creado_en"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    if (!insertCuenta.exec()) {
        m_lastError = insertCuenta.lastError().text();
        db.rollback();
        return false;
    }

    const std::int64_t cuentaId = insertCuenta.lastInsertId().toLongLong();
    if (!insertSaldoMes(cuentaId, mes, saldoInicialCentavos, saldoInicialCentavos)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }

    return true;
}

bool Database::actualizarCuenta(std::int64_t id, const QString &nombre, std::int64_t saldoInicialCentavos,
                                const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery updateCuenta(db);
    updateCuenta.prepare(QStringLiteral("UPDATE cuenta SET nombre = :nombre WHERE id = :id"));
    updateCuenta.bindValue(QStringLiteral(":nombre"), nombre);
    updateCuenta.bindValue(QStringLiteral(":id"), id);
    if (!updateCuenta.exec()) {
        m_lastError = updateCuenta.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery updateSaldo(db);
    updateSaldo.prepare(QStringLiteral(
        "UPDATE saldo_mes SET saldo_inicial = :saldo_inicial "
        "WHERE cuenta_id = :cuenta_id AND mes = :mes"));
    updateSaldo.bindValue(QStringLiteral(":saldo_inicial"), saldoInicialCentavos);
    updateSaldo.bindValue(QStringLiteral(":cuenta_id"), id);
    updateSaldo.bindValue(QStringLiteral(":mes"), mes);
    if (!updateSaldo.exec()) {
        m_lastError = updateSaldo.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }

    return true;
}

bool Database::eliminarCuenta(std::int64_t id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM cuenta WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool Database::crearMovimiento(std::int64_t cuentaId, const QDate &fecha, std::int64_t montoCentavos,
                               const QString &concepto, const QString &categoria)
{
    if (!fecha.isValid() || montoCentavos == 0) {
        m_lastError = QStringLiteral("Movimiento invalido.");
        return false;
    }

    const QString mes = fecha.toString(QStringLiteral("yyyy-MM"));
    const QString categoriaNormalizada = categoria.trimmed();
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    if (!ensureSaldoMesForCuenta(cuentaId, mes)) {
        db.rollback();
        return false;
    }

    QSqlQuery insertMovimiento(db);
    insertMovimiento.prepare(QStringLiteral(
        "INSERT INTO movimiento (cuenta_id, mes, fecha, monto, concepto, categoria) "
        "VALUES (:cuenta_id, :mes, :fecha, :monto, :concepto, :categoria)"));
    insertMovimiento.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    insertMovimiento.bindValue(QStringLiteral(":mes"), mes);
    insertMovimiento.bindValue(QStringLiteral(":fecha"), fecha.toString(Qt::ISODate));
    insertMovimiento.bindValue(QStringLiteral(":monto"), montoCentavos);
    insertMovimiento.bindValue(QStringLiteral(":concepto"), concepto);
    insertMovimiento.bindValue(QStringLiteral(":categoria"),
                               categoriaNormalizada.isEmpty() ? QVariant() : categoriaNormalizada);

    if (!insertMovimiento.exec()) {
        m_lastError = insertMovimiento.lastError().text();
        db.rollback();
        return false;
    }

    if (!aplicarMontoSaldo(cuentaId, mes, montoCentavos)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }

    return true;
}

bool Database::eliminarMovimiento(std::int64_t movimientoId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral(
        "SELECT cuenta_id, mes, monto FROM movimiento WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), movimientoId);
    if (!fetch.exec() || !fetch.next()) {
        m_lastError = fetch.lastError().text().isEmpty() ? QStringLiteral("Movimiento no encontrado.")
                                                         : fetch.lastError().text();
        return false;
    }

    const std::int64_t cuentaId = fetch.value(0).toLongLong();
    const QString mes = fetch.value(1).toString();
    const std::int64_t monto = fetch.value(2).toLongLong();

    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery remove(db);
    remove.prepare(QStringLiteral("DELETE FROM movimiento WHERE id = :id"));
    remove.bindValue(QStringLiteral(":id"), movimientoId);
    if (!remove.exec()) {
        m_lastError = remove.lastError().text();
        db.rollback();
        return false;
    }

    if (!aplicarMontoSaldo(cuentaId, mes, -monto)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }

    return true;
}

std::vector<Movimiento> Database::movimientosDeCuenta(std::int64_t cuentaId, const QString &mes)
{
    std::vector<Movimiento> movimientos;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT m.id, m.cuenta_id, m.mes, m.fecha, m.monto, m.concepto, m.categoria, c.moneda "
        "FROM movimiento m "
        "INNER JOIN cuenta c ON c.id = m.cuenta_id "
        "WHERE m.cuenta_id = :cuenta_id AND m.mes = :mes "
        "ORDER BY m.fecha DESC, m.id DESC"));
    query.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    query.bindValue(QStringLiteral(":mes"), mes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return movimientos;
    }

    while (query.next()) {
        Movimiento movimiento;
        movimiento.id = query.value(0).toLongLong();
        movimiento.cuentaId = query.value(1).toLongLong();
        movimiento.mes = query.value(2).toString();
        movimiento.fecha = QDate::fromString(query.value(3).toString(), Qt::ISODate);
        movimiento.monto = query.value(4).toLongLong();
        movimiento.concepto = query.value(5).toString();
        movimiento.categoria = query.value(6).toString();
        movimiento.moneda = monedaFromInt(query.value(7).toInt());
        movimientos.push_back(movimiento);
    }

    return movimientos;
}

QStringList Database::categoriasConocidas()
{
    QStringList categorias;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT DISTINCT categoria FROM movimiento "
            "WHERE categoria IS NOT NULL AND TRIM(categoria) != '' "
            "ORDER BY categoria COLLATE NOCASE"))) {
        m_lastError = query.lastError().text();
        return categorias;
    }

    while (query.next()) {
        categorias.append(query.value(0).toString());
    }

    return categorias;
}

std::vector<GastoCategoria> Database::gastoPorCategoria(const QString &mes, Moneda moneda)
{
    std::vector<GastoCategoria> gastos;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT CASE "
        "WHEN m.categoria IS NULL OR TRIM(m.categoria) = '' THEN :sin_categoria "
        "ELSE m.categoria END, "
        "SUM(ABS(m.monto)) "
        "FROM movimiento m "
        "INNER JOIN cuenta c ON c.id = m.cuenta_id "
        "WHERE m.mes = :mes AND c.moneda = :moneda AND m.monto < 0 "
        "GROUP BY 1 "
        "ORDER BY 2 DESC"));
    query.bindValue(QStringLiteral(":sin_categoria"), QStringLiteral("Sin categoría"));
    query.bindValue(QStringLiteral(":mes"), mes);
    query.bindValue(QStringLiteral(":moneda"), static_cast<int>(moneda));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return gastos;
    }

    while (query.next()) {
        GastoCategoria gasto;
        gasto.categoria = query.value(0).toString();
        gasto.totalCentavos = query.value(1).toLongLong();
        gastos.push_back(gasto);
    }

    return gastos;
}

std::optional<std::int64_t> Database::tasaCambio(const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT usd_a_ars FROM tasa_cambio WHERE mes = :mes"));
    query.bindValue(QStringLiteral(":mes"), mes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return query.value(0).toLongLong();
}

bool Database::guardarTasaCambio(const QString &mes, std::int64_t usdAArsCentavos)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO tasa_cambio (mes, usd_a_ars) VALUES (:mes, :usd_a_ars) "
        "ON CONFLICT(mes) DO UPDATE SET usd_a_ars = excluded.usd_a_ars"));
    query.bindValue(QStringLiteral(":mes"), mes);
    query.bindValue(QStringLiteral(":usd_a_ars"), usdAArsCentavos);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QStringList Database::mesesConDatos()
{
    QStringList meses;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT DISTINCT mes FROM saldo_mes ORDER BY mes DESC"))) {
        m_lastError = query.lastError().text();
        return meses;
    }

    while (query.next()) {
        meses.append(query.value(0).toString());
    }

    return meses;
}

std::vector<ResumenMes> Database::resumenHistorico()
{
    std::vector<ResumenMes> resumenes;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT s.mes, "
            "SUM(CASE WHEN c.moneda = 1 THEN s.saldo_inicial - s.saldo_actual ELSE 0 END), "
            "SUM(CASE WHEN c.moneda = 2 THEN s.saldo_inicial - s.saldo_actual ELSE 0 END) "
            "FROM saldo_mes s "
            "INNER JOIN cuenta c ON c.id = s.cuenta_id "
            "GROUP BY s.mes "
            "ORDER BY s.mes DESC"))) {
        m_lastError = query.lastError().text();
        return resumenes;
    }

    while (query.next()) {
        ResumenMes resumen;
        resumen.mes = query.value(0).toString();
        resumen.gastadoUsd = query.value(1).toLongLong();
        resumen.gastadoArs = query.value(2).toLongLong();
        resumen.tasa = tasaCambio(resumen.mes);
        resumenes.push_back(resumen);
    }

    return resumenes;
}

QString Database::lastError() const
{
    return m_lastError;
}

bool Database::initializeSchema()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS cuenta ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "nombre TEXT NOT NULL,"
            "moneda INTEGER NOT NULL CHECK (moneda IN (1, 2)),"
            "creado_en TEXT NOT NULL"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS saldo_mes ("
            "cuenta_id INTEGER NOT NULL,"
            "mes TEXT NOT NULL,"
            "saldo_inicial INTEGER NOT NULL,"
            "saldo_actual INTEGER NOT NULL,"
            "PRIMARY KEY (cuenta_id, mes),"
            "FOREIGN KEY (cuenta_id) REFERENCES cuenta(id) ON DELETE CASCADE"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS tasa_cambio ("
            "mes TEXT PRIMARY KEY,"
            "usd_a_ars INTEGER NOT NULL"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS movimiento ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "cuenta_id INTEGER NOT NULL,"
            "mes TEXT NOT NULL,"
            "fecha TEXT NOT NULL,"
            "monto INTEGER NOT NULL,"
            "concepto TEXT NOT NULL,"
            "categoria TEXT,"
            "FOREIGN KEY (cuenta_id) REFERENCES cuenta(id) ON DELETE CASCADE"
            ")"),
    };

    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            m_lastError = query.lastError().text();
            return false;
        }
    }

    return true;
}

bool Database::migrateSchema()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery info(db);
    if (!info.exec(QStringLiteral("PRAGMA table_info(movimiento)"))) {
        m_lastError = info.lastError().text();
        return false;
    }

    bool hasCategoria = false;
    while (info.next()) {
        if (info.value(1).toString() == QStringLiteral("categoria")) {
            hasCategoria = true;
            break;
        }
    }

    if (!hasCategoria) {
        QSqlQuery alter(db);
        if (!alter.exec(QStringLiteral("ALTER TABLE movimiento ADD COLUMN categoria TEXT"))) {
            m_lastError = alter.lastError().text();
            return false;
        }
    }

    // Movimientos de Fase 4: copiar concepto a categoría si no tenían una.
    QSqlQuery backfill(db);
    if (!backfill.exec(QStringLiteral(
            "UPDATE movimiento SET categoria = concepto "
            "WHERE (categoria IS NULL OR TRIM(categoria) = '') AND TRIM(concepto) != ''"))) {
        m_lastError = backfill.lastError().text();
        return false;
    }

    return true;
}

bool Database::insertSaldoMes(std::int64_t cuentaId, const QString &mes, std::int64_t saldoInicial,
                              std::int64_t saldoActual)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO saldo_mes (cuenta_id, mes, saldo_inicial, saldo_actual) "
        "VALUES (:cuenta_id, :mes, :saldo_inicial, :saldo_actual)"));
    query.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    query.bindValue(QStringLiteral(":mes"), mes);
    query.bindValue(QStringLiteral(":saldo_inicial"), saldoInicial);
    query.bindValue(QStringLiteral(":saldo_actual"), saldoActual);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::int64_t Database::saldoActualDelMesAnterior(std::int64_t cuentaId, const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT saldo_actual FROM saldo_mes "
        "WHERE cuenta_id = :cuenta_id AND mes < :mes "
        "ORDER BY mes DESC LIMIT 1"));
    query.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    query.bindValue(QStringLiteral(":mes"), mes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        return -1;
    }

    return query.value(0).toLongLong();
}

bool Database::ensureSaldoMesForCuenta(std::int64_t cuentaId, const QString &mes)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    QSqlQuery existsQuery(db);
    existsQuery.prepare(QStringLiteral(
        "SELECT 1 FROM saldo_mes WHERE cuenta_id = :cuenta_id AND mes = :mes"));
    existsQuery.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    existsQuery.bindValue(QStringLiteral(":mes"), mes);
    if (!existsQuery.exec()) {
        m_lastError = existsQuery.lastError().text();
        return false;
    }

    if (existsQuery.next()) {
        return true;
    }

    const std::int64_t saldoAnterior = saldoActualDelMesAnterior(cuentaId, mes);
    if (saldoAnterior < 0) {
        m_lastError = QStringLiteral("No hay saldo de referencia para crear el mes de la cuenta.");
        return false;
    }

    return insertSaldoMes(cuentaId, mes, saldoAnterior, saldoAnterior);
}

bool Database::aplicarMontoSaldo(std::int64_t cuentaId, const QString &mes, std::int64_t deltaCentavos)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE saldo_mes SET saldo_actual = saldo_actual + :delta "
        "WHERE cuenta_id = :cuenta_id AND mes = :mes"));
    query.bindValue(QStringLiteral(":delta"), deltaCentavos);
    query.bindValue(QStringLiteral(":cuenta_id"), cuentaId);
    query.bindValue(QStringLiteral(":mes"), mes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("No se pudo actualizar el saldo del mes.");
        return false;
    }

    return true;
}
