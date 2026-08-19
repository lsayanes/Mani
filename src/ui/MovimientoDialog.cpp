#include "ui/MovimientoDialog.h"

#include "model/Moneda.h"
#include "util/Money.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

MovimientoDialog::MovimientoDialog(const std::vector<Cuenta> &cuentas, const QDate &fechaDefault,
                                   const QStringList &categorias, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Nuevo movimiento"));
    resize(420, 300);

    m_cuentaCombo = new QComboBox(this);
    for (const Cuenta &cuenta : cuentas) {
        m_cuentaCombo->addItem(
            QStringLiteral("%1 (%2)").arg(cuenta.nombre, monedaLabel(cuenta.moneda)),
            QVariant::fromValue(cuenta.id));
    }

    m_tipoCombo = new QComboBox(this);
    m_tipoCombo->addItem(tr("Ingreso"), static_cast<int>(Tipo::Ingreso));
    m_tipoCombo->addItem(tr("Egreso"), static_cast<int>(Tipo::Egreso));

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

    m_conceptoEdit = new QLineEdit(this);
    m_conceptoEdit->setPlaceholderText(tr("Ej: Supermercado"));

    auto *form = new QFormLayout;
    form->addRow(tr("Cuenta"), m_cuentaCombo);
    form->addRow(tr("Tipo"), m_tipoCombo);
    form->addRow(tr("Fecha"), m_fechaEdit);
    form->addRow(tr("Monto"), m_montoEdit);
    form->addRow(tr("Concepto"), m_conceptoEdit);
    form->addRow(tr("Categoría"), m_categoriaCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &MovimientoDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &MovimientoDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void MovimientoDialog::setCuentaId(std::int64_t cuentaId)
{
    const int index = m_cuentaCombo->findData(QVariant::fromValue(cuentaId));
    if (index >= 0) {
        m_cuentaCombo->setCurrentIndex(index);
        m_cuentaCombo->setEnabled(false);
    }
}

void MovimientoDialog::setDatosEdicion(const Movimiento &movimiento)
{
    setWindowTitle(tr("Editar movimiento"));

    setCuentaId(movimiento.cuentaId);

    const bool ingreso = movimiento.monto >= 0;
    const int tipoIndex =
        m_tipoCombo->findData(static_cast<int>(ingreso ? Tipo::Ingreso : Tipo::Egreso));
    if (tipoIndex >= 0) {
        m_tipoCombo->setCurrentIndex(tipoIndex);
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
}

std::int64_t MovimientoDialog::cuentaId() const
{
    return m_cuentaCombo->currentData().toLongLong();
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
    const auto parsed = parseMoney(m_montoEdit->text());
    if (!parsed.has_value()) {
        return 0;
    }

    const std::int64_t absolute = parsed.value() < 0 ? -parsed.value() : parsed.value();
    return tipo() == Tipo::Ingreso ? absolute : -absolute;
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

    const auto parsed = parseMoney(m_montoEdit->text());
    if (!parsed.has_value() || *parsed == 0) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un monto valido mayor a cero."));
        return;
    }

    if (concepto().isEmpty()) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un concepto."));
        return;
    }

    QDialog::accept();
}
