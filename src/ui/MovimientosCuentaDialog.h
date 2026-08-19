#pragma once

#include "database/Database.h"
#include "model/Cuenta.h"

#include <vector>

#include <QDialog>

class QTableWidget;

class MovimientosCuentaDialog : public QDialog
{
    Q_OBJECT

public:
    MovimientosCuentaDialog(Database *database, const Cuenta &cuenta, const QString &mes,
                            const QDate &fechaDefault, QWidget *parent = nullptr);

signals:
    void datosModificados();

private slots:
    void onAgregar();
    void onEditar();
    void onEliminar();

private:
    void populateTable();

    Database *m_database = nullptr;
    Cuenta m_cuenta;
    QString m_mes;
    QDate m_fechaDefault;
    QTableWidget *m_table = nullptr;
    std::vector<Movimiento> m_movimientos;
};
