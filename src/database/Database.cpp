#include "database/Database.h"

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

    return initializeSchema();
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
                                std::int64_t saldoActualCentavos, const QString &mes)
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
        "UPDATE saldo_mes SET saldo_inicial = :saldo_inicial, saldo_actual = :saldo_actual "
        "WHERE cuenta_id = :cuenta_id AND mes = :mes"));
    updateSaldo.bindValue(QStringLiteral(":saldo_inicial"), saldoInicialCentavos);
    updateSaldo.bindValue(QStringLiteral(":saldo_actual"), saldoActualCentavos);
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
    };

    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            m_lastError = query.lastError().text();
            return false;
        }
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
