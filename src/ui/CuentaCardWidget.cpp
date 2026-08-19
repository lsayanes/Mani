#include "ui/CuentaCardWidget.h"

#include "util/Money.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CuentaCardWidget::CuentaCardWidget(const Cuenta &cuenta, QWidget *parent)
    : QWidget(parent)
    , m_cuenta(cuenta)
{
    m_nombreLabel = new QLabel(this);
    auto nombreFont = m_nombreLabel->font();
    nombreFont.setBold(true);
    m_nombreLabel->setFont(nombreFont);

    auto *editButton = new QPushButton(tr("Editar"), this);
    auto *movimientosButton = new QPushButton(tr("Movimientos"), this);
    auto *deleteButton = new QPushButton(tr("Borrar"), this);

    connect(editButton, &QPushButton::clicked, this, [this]() { emit editRequested(m_cuenta.id); });
    connect(movimientosButton, &QPushButton::clicked, this,
            [this]() { emit movimientosRequested(m_cuenta.id); });
    connect(deleteButton, &QPushButton::clicked, this, [this]() { emit deleteRequested(m_cuenta.id); });

    auto *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(editButton);
    buttonsLayout->addWidget(movimientosButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    layout->setContentsMargins(0, 0, 0, 16);
    layout->addWidget(m_nombreLabel);
    addMoneyRow(layout, tr("Inicial"), &m_inicialValue);
    addMoneyRow(layout, tr("Actual"), &m_actualValue);
    addMoneyRow(layout, tr("Gastado"), &m_gastadoValue);
    layout->addLayout(buttonsLayout);

    refresh();
}

QHBoxLayout *CuentaCardWidget::addMoneyRow(QVBoxLayout *layout, const QString &prefix, QLabel **valueOut)
{
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);

    auto *prefixLabel = new QLabel(prefix, this);
    prefixLabel->setMinimumWidth(56);

    auto *valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    row->addWidget(prefixLabel);
    row->addStretch();
    row->addWidget(valueLabel);
    layout->addLayout(row);

    *valueOut = valueLabel;
    return row;
}

void CuentaCardWidget::setCuenta(const Cuenta &cuenta)
{
    m_cuenta = cuenta;
    refresh();
}

std::int64_t CuentaCardWidget::cuentaId() const
{
    return m_cuenta.id;
}

void CuentaCardWidget::refresh()
{
    m_nombreLabel->setText(m_cuenta.nombre);
    styleMoneyValue(m_inicialValue, m_cuenta.saldoInicial);
    styleMoneyValue(m_actualValue, m_cuenta.saldoActual);
    styleMoneyValue(m_gastadoValue, m_cuenta.gastado);
}
