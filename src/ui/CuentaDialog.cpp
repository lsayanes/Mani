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
    resize(360, 200);

    m_nombreEdit = new QLineEdit(this);
    m_monedaCombo = new QComboBox(this);
    m_monedaCombo->addItem(QStringLiteral("USD"), static_cast<int>(Moneda::USD));
    m_monedaCombo->addItem(QStringLiteral("ARS"), static_cast<int>(Moneda::ARS));

    m_saldoInicialEdit = new QLineEdit(this);
    m_saldoInicialEdit->setPlaceholderText(tr("Ej: 1.000,00"));

    auto *form = new QFormLayout;
    form->addRow(tr("Nombre"), m_nombreEdit);
    form->addRow(tr("Moneda"), m_monedaCombo);
    form->addRow(tr("Saldo inicial"), m_saldoInicialEdit);

    if (mode == Mode::Edit) {
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

    QDialog::accept();
}
