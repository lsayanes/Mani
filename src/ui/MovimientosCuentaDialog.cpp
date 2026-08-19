#include "ui/MovimientosCuentaDialog.h"

#include "model/Movimiento.h"
#include "model/Moneda.h"
#include "ui/MovimientoDialog.h"
#include "util/Money.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

MovimientosCuentaDialog::MovimientosCuentaDialog(Database *database, const Cuenta &cuenta,
                                                 const QString &mes, const QDate &fechaDefault,
                                                 QWidget *parent)
    : QDialog(parent)
    , m_database(database)
    , m_cuenta(cuenta)
    , m_mes(mes)
    , m_fechaDefault(fechaDefault)
{
    setWindowTitle(tr("Movimientos — %1").arg(cuenta.nombre));
    resize(640, 420);

    auto *intro = new QLabel(tr("Mes: %1 — Moneda: %2").arg(mes, monedaLabel(cuenta.moneda)), this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Fecha"), tr("Concepto"), tr("Categoría"), tr("Monto")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    auto *agregarButton = new QPushButton(tr("Agregar movimiento"), this);
    auto *editarButton = new QPushButton(tr("Editar"), this);
    auto *eliminarButton = new QPushButton(tr("Eliminar"), this);
    connect(agregarButton, &QPushButton::clicked, this, &MovimientosCuentaDialog::onAgregar);
    connect(editarButton, &QPushButton::clicked, this, &MovimientosCuentaDialog::onEditar);
    connect(eliminarButton, &QPushButton::clicked, this, &MovimientosCuentaDialog::onEliminar);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MovimientosCuentaDialog::onEditar);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &MovimientosCuentaDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addWidget(agregarButton);
    layout->addWidget(editarButton);
    layout->addWidget(eliminarButton);
    layout->addWidget(buttons);

    populateTable();
}

void MovimientosCuentaDialog::populateTable()
{
    m_movimientos = m_database->movimientosDeCuenta(m_cuenta.id, m_mes);
    m_table->setRowCount(static_cast<int>(m_movimientos.size()));

    for (int row = 0; row < static_cast<int>(m_movimientos.size()); ++row) {
        const Movimiento &movimiento = m_movimientos[static_cast<std::size_t>(row)];

        auto *fechaItem = new QTableWidgetItem(movimiento.fecha.toString(QStringLiteral("dd/MM/yyyy")));
        fechaItem->setData(Qt::UserRole, movimiento.id);
        m_table->setItem(row, 0, fechaItem);
        m_table->setItem(row, 1, new QTableWidgetItem(movimiento.concepto));
        const QString categoriaTexto =
            movimiento.categoria.isEmpty() ? tr("Sin categoría") : movimiento.categoria;
        m_table->setItem(row, 2, new QTableWidgetItem(categoriaTexto));
        m_table->setItem(row, 3, new QTableWidgetItem(formatMoney(movimiento.monto)));
    }

    m_table->resizeColumnsToContents();
}

void MovimientosCuentaDialog::onAgregar()
{
    const std::vector<Cuenta> cuentas = {m_cuenta};
    MovimientoDialog dialog(cuentas, m_fechaDefault, m_database->categoriasConocidas(), this);
    dialog.setCuentaId(m_cuenta.id);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (!m_database->crearMovimiento(dialog.cuentaId(), dialog.fecha(), dialog.montoCentavos(),
                                     dialog.concepto(), dialog.categoria())) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    populateTable();
    emit datosModificados();
}

void MovimientosCuentaDialog::onEditar()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= static_cast<int>(m_movimientos.size())) {
        QMessageBox::information(this, tr("Editar movimiento"), tr("Selecciona un movimiento."));
        return;
    }

    const Movimiento &movimiento = m_movimientos[static_cast<std::size_t>(row)];

    const std::vector<Cuenta> cuentas = {m_cuenta};
    MovimientoDialog dialog(cuentas, m_fechaDefault, m_database->categoriasConocidas(), this);
    dialog.setDatosEdicion(movimiento);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (!m_database->actualizarMovimiento(movimiento.id, dialog.fecha(), dialog.montoCentavos(),
                                          dialog.concepto(), dialog.categoria())) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    populateTable();
    emit datosModificados();
}

void MovimientosCuentaDialog::onEliminar()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Eliminar movimiento"), tr("Selecciona un movimiento."));
        return;
    }

    QTableWidgetItem *fechaItem = m_table->item(row, 0);
    if (!fechaItem) {
        return;
    }

    const std::int64_t movimientoId = fechaItem->data(Qt::UserRole).toLongLong();
    const auto answer =
        QMessageBox::question(this, tr("Confirmar eliminacion"), tr("Queres eliminar este movimiento?"));
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_database->eliminarMovimiento(movimientoId)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    populateTable();
    emit datosModificados();
}
