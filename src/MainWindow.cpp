#include "MainWindow.h"

#include "model/Moneda.h"
#include "ui/CuentaCardWidget.h"
#include "ui/CuentaDialog.h"
#include "ui/TotalesPanelWidget.h"
#include "util/MesActivo.h"
#include "util/Totales.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QToolBar>
#include <QVBoxLayout>

#include <optional>

MainWindow::MainWindow(Database *database, QWidget *parent)
    : QMainWindow(parent)
    , m_database(database)
    , m_mesActivo(mesActivoActual())
{
    setWindowTitle(tr("Mani"));
    resize(900, 600);

    auto *toolbar = addToolBar(tr("Acciones"));
    auto *nuevaCuentaAction = toolbar->addAction(tr("Nueva cuenta"));
    connect(nuevaCuentaAction, &QAction::triggered, this, &MainWindow::onNuevaCuenta);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    m_mesLabel = new QLabel(this);
    rootLayout->addWidget(m_mesLabel);

    m_welcomeWidget = new QWidget(this);
    auto *welcomeLayout = new QVBoxLayout(m_welcomeWidget);
    welcomeLayout->setAlignment(Qt::AlignCenter);

    auto *welcomeTitle = new QLabel(tr("Bienvenido a Mani"), this);
    auto welcomeTitleFont = welcomeTitle->font();
    welcomeTitleFont.setPointSize(welcomeTitleFont.pointSize() + 4);
    welcomeTitleFont.setBold(true);
    welcomeTitle->setFont(welcomeTitleFont);
    welcomeTitle->setAlignment(Qt::AlignCenter);

    auto *welcomeText = new QLabel(
        tr("Todavia no hay cuentas cargadas.\n"
           "Crea tu primera cuenta para empezar a registrar saldos en USD y ARS."),
        this);
    welcomeText->setWordWrap(true);
    welcomeText->setAlignment(Qt::AlignCenter);

    auto *welcomeButton = new QPushButton(tr("Crear primera cuenta"), this);
    welcomeButton->setMinimumWidth(220);
    connect(welcomeButton, &QPushButton::clicked, this, &MainWindow::onNuevaCuenta);

    welcomeLayout->addStretch();
    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addSpacing(12);
    welcomeLayout->addWidget(welcomeText);
    welcomeLayout->addSpacing(16);
    welcomeLayout->addWidget(welcomeButton, 0, Qt::AlignCenter);
    welcomeLayout->addStretch();

    m_contentWidget = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    auto *columnsLayout = new QHBoxLayout;
    columnsLayout->setSpacing(24);

    m_usdColumn = new QWidget(this);
    m_arsColumn = new QWidget(this);
    m_usdLayout = new QVBoxLayout(m_usdColumn);
    m_arsLayout = new QVBoxLayout(m_arsColumn);
    m_usdLayout->setSpacing(12);
    m_arsLayout->setSpacing(12);

    auto *usdTitle = new QLabel(QStringLiteral("USD"), this);
    auto usdTitleFont = usdTitle->font();
    usdTitleFont.setBold(true);
    usdTitleFont.setPointSize(usdTitleFont.pointSize() + 1);
    usdTitle->setFont(usdTitleFont);
    m_usdLayout->addWidget(usdTitle);
    m_usdLayout->addStretch();

    auto *arsTitle = new QLabel(QStringLiteral("ARS"), this);
    auto arsTitleFont = arsTitle->font();
    arsTitleFont.setBold(true);
    arsTitleFont.setPointSize(arsTitleFont.pointSize() + 1);
    arsTitle->setFont(arsTitleFont);
    m_arsLayout->addWidget(arsTitle);
    m_arsLayout->addStretch();

    auto makeColumnScroll = [this](QWidget *column) {
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(column);
        return scroll;
    };

    columnsLayout->addWidget(makeColumnScroll(m_usdColumn), 1);
    columnsLayout->addWidget(makeColumnScroll(m_arsColumn), 1);
    contentLayout->addLayout(columnsLayout, 1);

    m_totalesPanel = new TotalesPanelWidget(this);
    connect(m_totalesPanel, &TotalesPanelWidget::tasaGuardada, this, &MainWindow::onTasaGuardada);
    contentLayout->addWidget(m_totalesPanel);

    rootLayout->addWidget(m_welcomeWidget, 1);
    rootLayout->addWidget(m_contentWidget, 1);

    setCentralWidget(central);
    refresh();
}

void MainWindow::onTasaGuardada(std::int64_t usdAArsCentavos)
{
    if (!m_database->guardarTasaCambio(m_mesActivo, usdAArsCentavos)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    refresh();
}

void MainWindow::onNuevaCuenta()
{
    CuentaDialog dialog(CuentaDialog::Mode::Create, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (!m_database->crearCuenta(dialog.nombre(), dialog.moneda(), dialog.saldoInicialCentavos(), m_mesActivo)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    refresh();
}

void MainWindow::onEditarCuenta(std::int64_t cuentaId)
{
    Cuenta *cuenta = findCuenta(cuentaId);
    if (!cuenta) {
        return;
    }

    CuentaDialog dialog(CuentaDialog::Mode::Edit, this);
    dialog.setCuenta(*cuenta);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (!m_database->actualizarCuenta(cuentaId, dialog.nombre(), dialog.saldoInicialCentavos(),
                                      dialog.saldoActualCentavos(), m_mesActivo)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    refresh();
}

void MainWindow::onBorrarCuenta(std::int64_t cuentaId)
{
    Cuenta *cuenta = findCuenta(cuentaId);
    if (!cuenta) {
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Confirmar borrado"),
        tr("Queres borrar la cuenta \"%1\"? Esta accion no se puede deshacer.").arg(cuenta->nombre));
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_database->eliminarCuenta(cuentaId)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    refresh();
}

void MainWindow::refresh()
{
    m_mesActivo = mesActivoActual();
    m_database->ensureMesActivo(m_mesActivo);
    m_cuentas = m_database->cuentasDelMes(m_mesActivo);

    m_mesLabel->setText(tr("Mes activo: %1").arg(m_mesActivo));

    const bool hasCuentas = !m_cuentas.empty();
    m_welcomeWidget->setVisible(!hasCuentas);
    m_contentWidget->setVisible(hasCuentas);

    clearColumnCards(m_usdLayout);
    clearColumnCards(m_arsLayout);

    for (const Cuenta &cuenta : m_cuentas) {
        auto *card = new CuentaCardWidget(cuenta, this);
        connect(card, &CuentaCardWidget::editRequested, this, &MainWindow::onEditarCuenta);
        connect(card, &CuentaCardWidget::deleteRequested, this, &MainWindow::onBorrarCuenta);

        if (cuenta.moneda == Moneda::USD) {
            m_usdLayout->insertWidget(m_usdLayout->count() - 1, card);
        } else {
            m_arsLayout->insertWidget(m_arsLayout->count() - 1, card);
        }
    }

    const TotalesMes totales = calcularTotales(m_cuentas);
    const std::optional<std::int64_t> tasa = m_database->tasaCambio(m_mesActivo);

    m_totalesPanel->setTasa(tasa);
    m_totalesPanel->setTotales(totales, tasa);
}

void MainWindow::clearColumnCards(QVBoxLayout *layout)
{
    // Conserva titulo (indice 0) y stretch (ultimo).
    while (layout->count() > 2) {
        QLayoutItem *item = layout->takeAt(1);
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

Cuenta *MainWindow::findCuenta(std::int64_t cuentaId)
{
    for (Cuenta &cuenta : m_cuentas) {
        if (cuenta.id == cuentaId) {
            return &cuenta;
        }
    }
    return nullptr;
}
