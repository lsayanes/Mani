#pragma once

#include "database/Database.h"

#include <QMainWindow>

class QAction;
class QComboBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

class TotalesPanelWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database *database, QWidget *parent = nullptr);

private slots:
    void onNuevaCuenta();
    void onEditarCuenta(std::int64_t cuentaId);
    void onBorrarCuenta(std::int64_t cuentaId);
    void onMovimientosCuenta(std::int64_t cuentaId);
    void onNuevoMovimiento();
    void onTasaGuardada(std::int64_t usdAArsCentavos);
    void onMesAnterior();
    void onMesSiguiente();
    void onMesHoy();
    void onMesComboChanged(int index);
    void onVerHistorial();

private:
    void refresh();
    void refreshMesSelector();
    void clearColumnCards(QVBoxLayout *layout);
    void irAMes(const QString &mes);
    bool esMesCalendarioActual() const;
    Cuenta *findCuenta(std::int64_t cuentaId);

    Database *m_database = nullptr;
    QString m_mesSeleccionado;
    std::vector<Cuenta> m_cuentas;

    QAction *m_nuevaCuentaAction = nullptr;
    QAction *m_nuevoMovimientoAction = nullptr;
    QPushButton *m_mesPrevButton = nullptr;
    QPushButton *m_mesNextButton = nullptr;
    QPushButton *m_mesHoyButton = nullptr;
    QComboBox *m_mesCombo = nullptr;
    QLabel *m_mesEstadoLabel = nullptr;
    QWidget *m_welcomeWidget = nullptr;
    QWidget *m_contentWidget = nullptr;
    QWidget *m_usdColumn = nullptr;
    QWidget *m_arsColumn = nullptr;
    QVBoxLayout *m_usdLayout = nullptr;
    QVBoxLayout *m_arsLayout = nullptr;
    TotalesPanelWidget *m_totalesPanel = nullptr;
};
