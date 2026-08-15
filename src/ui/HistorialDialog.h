#pragma once

#include "database/Database.h"

#include <QDialog>

class QTableWidget;

class HistorialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistorialDialog(Database *database, const QString &mesActual, QWidget *parent = nullptr);

signals:
    void mesSeleccionado(const QString &mes);

private:
    void populateTable();
    void onRowActivated(int row, int column);

    Database *m_database = nullptr;
    QString m_mesActual;
    QTableWidget *m_table = nullptr;
};
