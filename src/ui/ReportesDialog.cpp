#include "ui/ReportesDialog.h"

#include "model/GastoCategoria.h"
#include "model/Moneda.h"
#include "model/ResumenMes.h"
#include "ui/BarChartWidget.h"
#include "util/Money.h"
#include "util/Totales.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ReportesDialog::ReportesDialog(Database *database, const QString &mes, QWidget *parent)
    : QDialog(parent)
    , m_database(database)
    , m_mes(mes)
{
    setWindowTitle(tr("Reportes"));
    resize(760, 560);

    auto *intro = new QLabel(tr("Reportes para el mes %1").arg(mes), this);
    auto introFont = intro->font();
    introFont.setBold(true);
    intro->setFont(introFont);

    auto *tabs = new QTabWidget(this);
    buildCategoriaTab(tabs);
    buildMesTab(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &ReportesDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(tabs, 1);
    layout->addWidget(buttons);
}

void ReportesDialog::buildCategoriaTab(QTabWidget *tabs)
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    const std::vector<GastoCategoria> gastosUsd = m_database->gastoPorCategoria(m_mes, Moneda::USD);
    const std::vector<GastoCategoria> gastosArs = m_database->gastoPorCategoria(m_mes, Moneda::ARS);

    auto *usdChart = new BarChartWidget(tab);
    usdChart->setTitulo(QStringLiteral("USD"));
    usdChart->setDatos(gastosUsd);

    auto *usdTable = new QTableWidget(tab);
    usdTable->setColumnCount(2);
    usdTable->setHorizontalHeaderLabels({tr("Categoría"), tr("Gasto")});
    usdTable->horizontalHeader()->setStretchLastSection(true);
    usdTable->verticalHeader()->setVisible(false);
    usdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    populateCategoriaTable(usdTable, gastosUsd);

    auto *arsChart = new BarChartWidget(tab);
    arsChart->setTitulo(QStringLiteral("ARS"));
    arsChart->setDatos(gastosArs);

    auto *arsTable = new QTableWidget(tab);
    arsTable->setColumnCount(2);
    arsTable->setHorizontalHeaderLabels({tr("Categoría"), tr("Gasto")});
    arsTable->horizontalHeader()->setStretchLastSection(true);
    arsTable->verticalHeader()->setVisible(false);
    arsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    populateCategoriaTable(arsTable, gastosArs);

    layout->addWidget(new QLabel(tr("Gasto por categoría (egresos del mes)"), tab));
    layout->addWidget(usdChart);
    layout->addWidget(usdTable);
    layout->addWidget(arsChart);
    layout->addWidget(arsTable);

    tabs->addTab(tab, tr("Por categoría"));
}

void ReportesDialog::buildMesTab(QTabWidget *tabs)
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    const std::vector<ResumenMes> resumenes = m_database->resumenHistorico();

    std::vector<GastoCategoria> gastosMensuales;
    gastosMensuales.reserve(resumenes.size());
    for (const ResumenMes &resumen : resumenes) {
        GastoCategoria item;
        item.categoria = resumen.mes;
        if (resumen.tasa.has_value()) {
            item.totalCentavos =
                consolidadoEnArs(resumen.gastadoArs, resumen.gastadoUsd, *resumen.tasa);
        } else {
            item.totalCentavos = resumen.gastadoArs;
        }
        gastosMensuales.push_back(item);
    }

    auto *chart = new BarChartWidget(tab);
    chart->setTitulo(tr("Gasto consolidado por mes"));
    chart->setDatos(gastosMensuales);

    auto *table = new QTableWidget(tab);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        {tr("Mes"), tr("Gastado USD"), tr("Gastado ARS"), tr("Gastado consolidado")});
    table->setRowCount(static_cast<int>(resumenes.size()));
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int row = 0; row < static_cast<int>(resumenes.size()); ++row) {
        const ResumenMes &resumen = resumenes[static_cast<std::size_t>(row)];
        table->setItem(row, 0, new QTableWidgetItem(resumen.mes));
        table->setItem(row, 1, new QTableWidgetItem(formatMoney(resumen.gastadoUsd)));
        table->setItem(row, 2, new QTableWidgetItem(formatMoney(resumen.gastadoArs)));

        QString consolidadoText = tr("Sin tasa");
        if (resumen.tasa.has_value()) {
            consolidadoText = formatMoney(
                consolidadoEnArs(resumen.gastadoArs, resumen.gastadoUsd, *resumen.tasa));
        }
        table->setItem(row, 3, new QTableWidgetItem(consolidadoText));
    }

    table->resizeColumnsToContents();

    layout->addWidget(new QLabel(tr("Comparativa mensual de gastos"), tab));
    layout->addWidget(chart, 1);
    layout->addWidget(table);

    tabs->addTab(tab, tr("Por mes"));
}

void ReportesDialog::populateCategoriaTable(QTableWidget *table,
                                            const std::vector<GastoCategoria> &datos)
{
    table->setRowCount(static_cast<int>(datos.size()));
    for (int row = 0; row < static_cast<int>(datos.size()); ++row) {
        const GastoCategoria &dato = datos[static_cast<std::size_t>(row)];
        table->setItem(row, 0, new QTableWidgetItem(dato.categoria));
        table->setItem(row, 1, new QTableWidgetItem(formatMoney(dato.totalCentavos)));
    }
    table->resizeColumnsToContents();
}
