#include "ui/HistorialDialog.h"

#include "model/ResumenMes.h"
#include "util/MesActivo.h"
#include "util/Money.h"
#include "util/Totales.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HistorialDialog::HistorialDialog(Database *database, const QString &mesActual, QWidget *parent)
    : QDialog(parent)
    , m_database(database)
    , m_mesActual(mesActual)
{
    setWindowTitle(tr("Historial mensual"));
    resize(720, 420);

    auto *intro = new QLabel(
        tr("Comparativa de gastos por mes. Doble clic en un mes para abrirlo."), this);
    intro->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(
        {tr("Mes"), tr("Gastado USD"), tr("Gastado ARS"), tr("Gastado consolidado")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &HistorialDialog::onRowActivated);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &HistorialDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    populateTable();
}

void HistorialDialog::populateTable()
{
    const std::vector<ResumenMes> resumenes = m_database->resumenHistorico();
    m_table->setRowCount(static_cast<int>(resumenes.size()));

    for (int row = 0; row < static_cast<int>(resumenes.size()); ++row) {
        const ResumenMes &resumen = resumenes[static_cast<std::size_t>(row)];

        QString mesLabel = resumen.mes;
        if (resumen.mes == mesActivoActual()) {
            mesLabel += tr(" (actual)");
        } else if (resumen.mes == m_mesActual) {
            mesLabel += tr(" (viendo)");
        }

        auto *mesItem = new QTableWidgetItem(mesLabel);
        if (resumen.mes == m_mesActual) {
            mesItem->setFont([] {
                QFont font;
                font.setBold(true);
                return font;
            }());
        }
        m_table->setItem(row, 0, mesItem);
        m_table->setItem(row, 1, new QTableWidgetItem(formatMoney(resumen.gastadoUsd)));
        m_table->setItem(row, 2, new QTableWidgetItem(formatMoney(resumen.gastadoArs)));

        QString consolidadoText = tr("Sin tasa");
        if (resumen.tasa.has_value()) {
            consolidadoText = formatMoney(
                consolidadoEnArs(resumen.gastadoArs, resumen.gastadoUsd, *resumen.tasa));
        }
        m_table->setItem(row, 3, new QTableWidgetItem(consolidadoText));
    }

    m_table->resizeColumnsToContents();
}

void HistorialDialog::onRowActivated(int row, int /*column*/)
{
    QTableWidgetItem *mesItem = m_table->item(row, 0);
    if (!mesItem) {
        return;
    }

    QString mes = mesItem->text();
    const int marker = mes.indexOf(QLatin1Char(' '));
    if (marker > 0) {
        mes = mes.left(marker);
    }

    if (!esMesValido(mes)) {
        return;
    }

    emit mesSeleccionado(mes);
    accept();
}
