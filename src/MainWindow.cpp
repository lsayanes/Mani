#include "MainWindow.h"

#include "model/Moneda.h"
#include "ui/CuentaCardWidget.h"
#include "ui/CuentaDialog.h"
#include "ui/HistorialDialog.h"
#include "ui/TotalesPanelWidget.h"
#include "util/MesActivo.h"
#include "util/Totales.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QToolBar>
#include <QVBoxLayout>

#include <optional>
#include <vector>

#include <algorithm>

MainWindow::MainWindow(Database *database, QWidget *parent)
    : QMainWindow(parent)
    , m_database(database)
    , m_mesSeleccionado(mesActivoActual())
{
    setWindowTitle(tr("Mani"));
    resize(900, 640);

    m_database->ensureMesActivo(mesActivoActual());

    auto *toolbar = addToolBar(tr("Acciones"));
    m_nuevaCuentaAction = toolbar->addAction(tr("Nueva cuenta"));
    connect(m_nuevaCuentaAction, &QAction::triggered, this, &MainWindow::onNuevaCuenta);
    toolbar->addAction(tr("Historial"), this, &MainWindow::onVerHistorial);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    auto *mesSelectorRow = new QHBoxLayout;
    mesSelectorRow->addWidget(new QLabel(tr("Mes:"), this));

    m_mesPrevButton = new QPushButton(QStringLiteral("◀"), this);
    m_mesPrevButton->setFixedWidth(36);
    connect(m_mesPrevButton, &QPushButton::clicked, this, &MainWindow::onMesAnterior);

    m_mesCombo = new QComboBox(this);
    m_mesCombo->setMinimumWidth(120);
    connect(m_mesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onMesComboChanged);

    m_mesNextButton = new QPushButton(QStringLiteral("▶"), this);
    m_mesNextButton->setFixedWidth(36);
    connect(m_mesNextButton, &QPushButton::clicked, this, &MainWindow::onMesSiguiente);

    m_mesHoyButton = new QPushButton(tr("Hoy"), this);
    connect(m_mesHoyButton, &QPushButton::clicked, this, &MainWindow::onMesHoy);

    m_mesEstadoLabel = new QLabel(this);

    mesSelectorRow->addWidget(m_mesPrevButton);
    mesSelectorRow->addWidget(m_mesCombo);
    mesSelectorRow->addWidget(m_mesNextButton);
    mesSelectorRow->addWidget(m_mesHoyButton);
    mesSelectorRow->addWidget(m_mesEstadoLabel);
    mesSelectorRow->addStretch();
    rootLayout->addLayout(mesSelectorRow);

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
    if (!m_database->guardarTasaCambio(m_mesSeleccionado, usdAArsCentavos)) {
        QMessageBox::critical(this, tr("Error"), m_database->lastError());
        return;
    }

    refresh();
}

void MainWindow::onNuevaCuenta()
{
    if (!esMesCalendarioActual()) {
        QMessageBox::information(this, tr("Mes historico"),
                                 tr("Solo podes crear cuentas en el mes calendario actual."));
        return;
    }

    CuentaDialog dialog(CuentaDialog::Mode::Create, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (!m_database->crearCuenta(dialog.nombre(), dialog.moneda(), dialog.saldoInicialCentavos(),
                                 m_mesSeleccionado)) {
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
                                      dialog.saldoActualCentavos(), m_mesSeleccionado)) {
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

    if (!esMesCalendarioActual()) {
        QMessageBox::information(this, tr("Mes historico"),
                                 tr("Solo podes borrar cuentas en el mes calendario actual."));
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

void MainWindow::onMesAnterior()
{
    const QString anterior = mesAnterior(m_mesSeleccionado);
    if (anterior.isEmpty()) {
        return;
    }
    irAMes(anterior);
}

void MainWindow::onMesSiguiente()
{
    const QString siguiente = mesSiguiente(m_mesSeleccionado);
    if (siguiente.isEmpty()) {
        return;
    }
    if (compararMeses(siguiente, mesActivoActual()) > 0) {
        return;
    }
    irAMes(siguiente);
}

void MainWindow::onMesHoy()
{
    irAMes(mesActivoActual());
}

void MainWindow::onMesComboChanged(int index)
{
    if (index < 0) {
        return;
    }

    const QString mes = m_mesCombo->itemData(index).toString();
    if (mes.isEmpty() || mes == m_mesSeleccionado) {
        return;
    }

    irAMes(mes);
}

void MainWindow::onVerHistorial()
{
    HistorialDialog dialog(m_database, m_mesSeleccionado, this);
    connect(&dialog, &HistorialDialog::mesSeleccionado, this, &MainWindow::irAMes);
    dialog.exec();
}

void MainWindow::refresh()
{
    m_database->ensureMesActivo(mesActivoActual());

    m_cuentas = m_database->cuentasDelMes(m_mesSeleccionado);
    refreshMesSelector();

    const bool hasCuentasGlobales = !m_database->mesesConDatos().isEmpty();
    const bool hasCuentasMes = !m_cuentas.empty();
    m_welcomeWidget->setVisible(!hasCuentasGlobales);
    m_contentWidget->setVisible(hasCuentasGlobales);

    if (!hasCuentasGlobales) {
        return;
    }

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
    const std::optional<std::int64_t> tasa = m_database->tasaCambio(m_mesSeleccionado);

    m_totalesPanel->setTasa(tasa);
    m_totalesPanel->setTotales(totales, tasa);

    m_nuevaCuentaAction->setEnabled(esMesCalendarioActual());
}

void MainWindow::refreshMesSelector()
{
    const QSignalBlocker blocker(m_mesCombo);

    m_mesCombo->clear();

    std::vector<QString> mesesOrdenados;
    for (const QString &mes : m_database->mesesConDatos()) {
        mesesOrdenados.push_back(mes);
    }
    mesesOrdenados.push_back(mesActivoActual());
    mesesOrdenados.push_back(m_mesSeleccionado);

    std::sort(mesesOrdenados.begin(), mesesOrdenados.end(),
              [](const QString &a, const QString &b) { return compararMeses(a, b) > 0; });
    mesesOrdenados.erase(std::unique(mesesOrdenados.begin(), mesesOrdenados.end()), mesesOrdenados.end());

    for (const QString &mes : mesesOrdenados) {
        QString label = mes;
        if (mes == mesActivoActual()) {
            label += tr(" (actual)");
        }
        m_mesCombo->addItem(label, mes);
    }

    const int index = m_mesCombo->findData(m_mesSeleccionado);
    if (index >= 0) {
        m_mesCombo->setCurrentIndex(index);
    }

    const QString anterior = mesAnterior(m_mesSeleccionado);
    m_mesPrevButton->setEnabled(!anterior.isEmpty());

    const QString siguiente = mesSiguiente(m_mesSeleccionado);
    const bool puedeAvanzar =
        !siguiente.isEmpty() && compararMeses(siguiente, mesActivoActual()) <= 0;
    m_mesNextButton->setEnabled(puedeAvanzar);

    m_mesHoyButton->setEnabled(!esMesCalendarioActual());

    if (esMesCalendarioActual()) {
        m_mesEstadoLabel->setText(tr("Mes calendario actual"));
    } else {
        m_mesEstadoLabel->setText(tr("Viendo mes historico"));
    }
}

void MainWindow::clearColumnCards(QVBoxLayout *layout)
{
    while (layout->count() > 2) {
        QLayoutItem *item = layout->takeAt(1);
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

void MainWindow::irAMes(const QString &mes)
{
    if (!esMesValido(mes)) {
        return;
    }

    m_mesSeleccionado = mes;
    refresh();
}

bool MainWindow::esMesCalendarioActual() const
{
    return m_mesSeleccionado == mesActivoActual();
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
