#pragma once

#include "database/Database.h"

#include <QMainWindow>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database *database, QWidget *parent = nullptr);

private slots:
    void onNuevaCuenta();
    void onEditarCuenta(std::int64_t cuentaId);
    void onBorrarCuenta(std::int64_t cuentaId);

private:
    void refresh();
    Cuenta *findCuenta(std::int64_t cuentaId);

    Database *m_database = nullptr;
    QString m_mesActivo;
    std::vector<Cuenta> m_cuentas;

    QLabel *m_mesLabel = nullptr;
    QWidget *m_welcomeWidget = nullptr;
    QWidget *m_contentWidget = nullptr;
    QWidget *m_usdColumn = nullptr;
    QWidget *m_arsColumn = nullptr;
    QVBoxLayout *m_usdLayout = nullptr;
    QVBoxLayout *m_arsLayout = nullptr;
};
