#include "ui/MovimientoDialog.h"

#include "model/Moneda.h"
#include "util/Money.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

MovimientoDialog::MovimientoDialog(const std::vector<Cuenta> &cuentas, const QDate &fechaDefault,
                                   const QStringList &categorias, QWidget *parent)
    : QDialog(parent)
    , m_cuentas(cuentas)
{
    setWindowTitle(tr("Nuevo movimiento"));
    resize(440, 360);

    m_cuentaCombo = new QComboBox(this);
    for (const Cuenta &cuenta : m_cuentas) {
        m_cuentaCombo->addItem(
            QStringLiteral("%1 (%2)").arg(cuenta.nombre, monedaLabel(cuenta.moneda)),
            QVariant::fromValue(cuenta.id));
    }

    m_destinoCombo = new QComboBox(this);

    m_tipoCombo = new QComboBox(this);
    m_tipoCombo->addItem(tr("Ingreso"), static_cast<int>(Tipo::Ingreso));
    m_tipoCombo->addItem(tr("Egreso"), static_cast<int>(Tipo::Egreso));
    m_tipoCombo->addItem(tr("Transferencia"), static_cast<int>(Tipo::Transferencia));
    m_tipoCombo->setCurrentIndex(1);

    m_categoriaCombo = new QComboBox(this);
    m_categoriaCombo->setEditable(true);
    m_categoriaCombo->addItem(tr("Sin categoría"), QString());
    for (const QString &categoria : categorias) {
        m_categoriaCombo->addItem(categoria, categoria);
    }

    m_fechaEdit = new QDateEdit(fechaDefault, this);
    m_fechaEdit->setCalendarPopup(true);
    m_fechaEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    m_montoEdit = new QLineEdit(this);
    m_montoEdit->setPlaceholderText(tr("Ej: 1.000,00"));

    m_montoDestinoEdit = new QLineEdit(this);
    m_montoDestinoEdit->setPlaceholderText(tr("Monto que entra en la cuenta destino"));

    m_conceptoEdit = new QLineEdit(this);
    m_conceptoEdit->setPlaceholderText(tr("Ej: Supermercado"));

    m_cuentaLabel = new QLabel(tr("Cuenta"), this);
    m_destinoLabel = new QLabel(tr("Hacia"), this);
    m_montoLabel = new QLabel(tr("Monto"), this);
    m_montoDestinoLabel = new QLabel(tr("Monto destino"), this);

    m_form = new QFormLayout;
    m_form->addRow(m_cuentaLabel, m_cuentaCombo);
    m_form->addRow(tr("Tipo"), m_tipoCombo);
    m_form->addRow(m_destinoLabel, m_destinoCombo);
    m_form->addRow(tr("Fecha"), m_fechaEdit);
    m_form->addRow(m_montoLabel, m_montoEdit);
    m_form->addRow(m_montoDestinoLabel, m_montoDestinoEdit);
    m_form->addRow(tr("Concepto"), m_conceptoEdit);
    m_form->addRow(tr("Categoría"), m_categoriaCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &MovimientoDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &MovimientoDialog::reject);

    connect(m_tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MovimientoDialog::updateTransferUi);
    connect(m_cuentaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MovimientoDialog::updateTransferUi);
    connect(m_destinoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MovimientoDialog::updateTransferUi);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(m_form);
    layout->addWidget(buttons);

    updateTransferUi();
}

void MovimientoDialog::setCuentaId(std::int64_t cuentaId)
{
    const int index = m_cuentaCombo->findData(QVariant::fromValue(cuentaId));
    if (index >= 0) {
        m_cuentaCombo->setCurrentIndex(index);
        m_cuentaCombo->setEnabled(false);
    }
    updateTransferUi();
}

void MovimientoDialog::setDatosEdicion(const Movimiento &movimiento)
{
    setWindowTitle(tr("Editar movimiento"));
    m_editando = true;
    m_montoOriginal = movimiento.monto;

    setCuentaId(movimiento.cuentaId);

    const int transferenciaIndex =
        m_tipoCombo->findData(static_cast<int>(Tipo::Transferencia));
    if (!movimiento.esTransferencia && transferenciaIndex >= 0) {
        m_tipoCombo->removeItem(transferenciaIndex);
    }

    const Tipo tipo = movimiento.esTransferencia
                          ? Tipo::Transferencia
                          : (movimiento.monto >= 0 ? Tipo::Ingreso : Tipo::Egreso);
    const int tipoIndex = m_tipoCombo->findData(static_cast<int>(tipo));
    if (tipoIndex >= 0) {
        m_tipoCombo->setCurrentIndex(tipoIndex);
    }
    if (movimiento.esTransferencia) {
        m_tipoCombo->setEnabled(false);
    }

    m_fechaEdit->setDate(movimiento.fecha);
    m_montoEdit->setText(formatMoney(movimiento.monto < 0 ? -movimiento.monto : movimiento.monto));
    m_conceptoEdit->setText(movimiento.concepto);

    const int categoriaIndex = m_categoriaCombo->findData(movimiento.categoria);
    if (categoriaIndex >= 0) {
        m_categoriaCombo->setCurrentIndex(categoriaIndex);
    } else {
        m_categoriaCombo->setCurrentText(movimiento.categoria);
    }

    updateTransferUi();
}

std::int64_t MovimientoDialog::cuentaId() const
{
    return m_cuentaCombo->currentData().toLongLong();
}

std::int64_t MovimientoDialog::cuentaDestinoId() const
{
    return m_destinoCombo->currentData().toLongLong();
}

QDate MovimientoDialog::fecha() const
{
    return m_fechaEdit->date();
}

MovimientoDialog::Tipo MovimientoDialog::tipo() const
{
    return static_cast<Tipo>(m_tipoCombo->currentData().toInt());
}

std::int64_t MovimientoDialog::montoCentavos() const
{
    const auto parsed = parseMontoAbsoluto(m_montoEdit->text());
    if (!parsed.has_value()) {
        return 0;
    }

    if (tipo() == Tipo::Ingreso) {
        return *parsed;
    }
    if (tipo() == Tipo::Egreso) {
        return -*parsed;
    }

    return m_montoOriginal < 0 ? -*parsed : *parsed;
}

std::int64_t MovimientoDialog::montoOrigenCentavos() const
{
    return parseMontoAbsoluto(m_montoEdit->text()).value_or(0);
}

std::int64_t MovimientoDialog::montoDestinoCentavos() const
{
    const Cuenta *origen = cuentaPorId(cuentaId());
    const Cuenta *destino = cuentaPorId(cuentaDestinoId());
    if (origen && destino && origen->moneda == destino->moneda) {
        return montoOrigenCentavos();
    }
    return parseMontoAbsoluto(m_montoDestinoEdit->text()).value_or(0);
}

QString MovimientoDialog::concepto() const
{
    return m_conceptoEdit->text().trimmed();
}

QString MovimientoDialog::categoria() const
{
    const QString texto = m_categoriaCombo->currentText().trimmed();
    if (texto.isEmpty() || texto == tr("Sin categoría")) {
        return {};
    }
    return texto;
}

void MovimientoDialog::updateTransferUi()
{
    const bool transferencia = !m_editando && tipo() == Tipo::Transferencia;
    m_cuentaLabel->setText(transferencia ? tr("Desde") : tr("Cuenta"));
    m_montoLabel->setText(transferencia ? tr("Monto origen") : tr("Monto"));
    m_conceptoEdit->setPlaceholderText(transferencia ? tr("Ej: Venta de dolares")
                                                     : tr("Ej: Supermercado"));

    m_destinoLabel->setVisible(transferencia);
    m_destinoCombo->setVisible(transferencia);

    const std::int64_t origenId = cuentaId();
    const std::int64_t destinoSeleccionado = m_destinoCombo->currentData().toLongLong();
    m_destinoCombo->blockSignals(true);
    m_destinoCombo->clear();
    for (const Cuenta &cuenta : m_cuentas) {
        if (cuenta.id == origenId) {
            continue;
        }
        m_destinoCombo->addItem(
            QStringLiteral("%1 (%2)").arg(cuenta.nombre, monedaLabel(cuenta.moneda)),
            QVariant::fromValue(cuenta.id));
    }
    const int destinoIndex = m_destinoCombo->findData(QVariant::fromValue(destinoSeleccionado));
    if (destinoIndex >= 0) {
        m_destinoCombo->setCurrentIndex(destinoIndex);
    }
    m_destinoCombo->blockSignals(false);

    const Cuenta *origen = cuentaPorId(origenId);
    const Cuenta *destino = cuentaPorId(cuentaDestinoId());
    const bool monedasDistintas =
        transferencia && origen && destino && origen->moneda != destino->moneda;

    m_montoDestinoLabel->setVisible(monedasDistintas);
    m_montoDestinoEdit->setVisible(monedasDistintas);
    if (monedasDistintas && destino) {
        m_montoDestinoLabel->setText(tr("Monto destino (%1)").arg(monedaLabel(destino->moneda)));
        if (origen) {
            m_montoLabel->setText(tr("Monto origen (%1)").arg(monedaLabel(origen->moneda)));
        }
    }
}

const Cuenta *MovimientoDialog::cuentaPorId(std::int64_t cuentaId) const
{
    for (const Cuenta &cuenta : m_cuentas) {
        if (cuenta.id == cuentaId) {
            return &cuenta;
        }
    }
    return nullptr;
}

std::optional<std::int64_t> MovimientoDialog::parseMontoAbsoluto(const QString &texto) const
{
    const auto parsed = parseMoney(texto);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    return *parsed < 0 ? -*parsed : *parsed;
}

void MovimientoDialog::accept()
{
    if (m_cuentaCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Selecciona una cuenta."));
        return;
    }

    if (!m_fechaEdit->date().isValid()) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa una fecha valida."));
        return;
    }

    if (montoOrigenCentavos() == 0) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un monto valido mayor a cero."));
        return;
    }

    if (tipo() == Tipo::Transferencia && !m_editando) {
        if (m_cuentas.size() < 2) {
            QMessageBox::warning(this, tr("Datos invalidos"),
                                 tr("Hace falta al menos otra cuenta para transferir."));
            return;
        }
        if (cuentaDestinoId() == 0 || cuentaDestinoId() == cuentaId()) {
            QMessageBox::warning(this, tr("Datos invalidos"), tr("Selecciona una cuenta destino."));
            return;
        }
        if (montoDestinoCentavos() == 0) {
            QMessageBox::warning(this, tr("Datos invalidos"),
                                 tr("Ingresa el monto que entra en la cuenta destino."));
            return;
        }
    }

    if (concepto().isEmpty()) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un concepto."));
        return;
    }

    QDialog::accept();
}
