#pragma once

#include "model/GastoCategoria.h"
#include "database/Database.h"

#include <QDialog>

class BarChartWidget;
class QTabWidget;
class QTableWidget;

class ReportesDialog : public QDialog
{
    Q_OBJECT

public:
    ReportesDialog(Database *database, const QString &mes, QWidget *parent = nullptr);

private:
    void buildCategoriaTab(QTabWidget *tabs);
    void buildMesTab(QTabWidget *tabs);
    void populateCategoriaTable(QTableWidget *table, const std::vector<GastoCategoria> &datos);

    Database *m_database = nullptr;
    QString m_mes;
};
