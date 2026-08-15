#include "ui/CuentaDialog.h"

#include "model/Moneda.h"
#include "util/Money.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

CuentaDialog::CuentaDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
{
    setWindowTitle(mode == Mode::Create ? tr("Nueva cuenta") : tr("Editar cuenta"));
    resize(360, 220);

    m_nombreEdit = new QLineEdit(this);
    m_monedaCombo = new QComboBox(this);
    m_monedaCombo->addItem(QStringLiteral("USD"), static_cast<int>(Moneda::USD));
    m_monedaCombo->addItem(QStringLiteral("ARS"), static_cast<int>(Moneda::ARS));

    m_saldoInicialEdit = new QLineEdit(this);
    m_saldoInicialEdit->setPlaceholderText(tr("Ej: 1.000,00"));

    m_saldoActualEdit = new QLineEdit(this);
    m_saldoActualEdit->setPlaceholderText(tr("Ej: 800,00"));

    auto *form = new QFormLayout;
    form->addRow(tr("Nombre"), m_nombreEdit);
    form->addRow(tr("Moneda"), m_monedaCombo);
    form->addRow(tr("Saldo inicial"), m_saldoInicialEdit);

    if (mode == Mode::Create) {
        m_saldoActualEdit->hide();
    } else {
        form->addRow(tr("Saldo actual"), m_saldoActualEdit);
        m_monedaCombo->setEnabled(false);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &CuentaDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &CuentaDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void CuentaDialog::setCuenta(const Cuenta &cuenta)
{
    m_nombreEdit->setText(cuenta.nombre);

    const int monedaIndex = m_monedaCombo->findData(static_cast<int>(cuenta.moneda));
    if (monedaIndex >= 0) {
        m_monedaCombo->setCurrentIndex(monedaIndex);
    }

    m_saldoInicialEdit->setText(formatMoney(cuenta.saldoInicial));
    if (m_mode == Mode::Edit) {
        m_saldoActualEdit->setText(formatMoney(cuenta.saldoActual));
    }
}

QString CuentaDialog::nombre() const
{
    return m_nombreEdit->text().trimmed();
}

Moneda CuentaDialog::moneda() const
{
    return static_cast<Moneda>(m_monedaCombo->currentData().toInt());
}

std::int64_t CuentaDialog::saldoInicialCentavos() const
{
    const auto parsed = parseMoney(m_saldoInicialEdit->text());
    return parsed.value_or(0);
}

std::int64_t CuentaDialog::saldoActualCentavos() const
{
    if (m_mode == Mode::Create) {
        return saldoInicialCentavos();
    }

    const auto parsed = parseMoney(m_saldoActualEdit->text());
    return parsed.value_or(0);
}

void CuentaDialog::accept()
{
    if (nombre().isEmpty()) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("El nombre no puede estar vacio."));
        return;
    }

    if (!parseMoney(m_saldoInicialEdit->text())) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un saldo inicial valido."));
        return;
    }

    if (m_mode == Mode::Edit && !parseMoney(m_saldoActualEdit->text())) {
        QMessageBox::warning(this, tr("Datos invalidos"), tr("Ingresa un saldo actual valido."));
        return;
    }

    QDialog::accept();
}
