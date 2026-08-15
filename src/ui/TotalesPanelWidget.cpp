#include "ui/TotalesPanelWidget.h"

#include "util/Money.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

TotalesPanelWidget::TotalesPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(8);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    auto *tasaTitle = new QLabel(tr("Tipo de cambio"), this);
    auto tasaTitleFont = tasaTitle->font();
    tasaTitleFont.setBold(true);
    tasaTitle->setFont(tasaTitleFont);

    m_tasaEdit = new QLineEdit(this);
    m_tasaEdit->setPlaceholderText(tr("Ej: 1.500,00"));
    m_tasaEdit->setMaximumWidth(160);

    auto *guardarTasaButton = new QPushButton(tr("Guardar"), this);
    connect(guardarTasaButton, &QPushButton::clicked, this, &TotalesPanelWidget::onGuardarTasa);
    connect(m_tasaEdit, &QLineEdit::returnPressed, this, &TotalesPanelWidget::onGuardarTasa);

    auto *tasaRow = new QHBoxLayout;
    tasaRow->addWidget(new QLabel(tr("1 USD = $"), this));
    tasaRow->addWidget(m_tasaEdit);
    tasaRow->addWidget(guardarTasaButton);
    tasaRow->addStretch();

    layout->addWidget(tasaTitle);
    layout->addLayout(tasaRow);

    auto *totalesTitle = new QLabel(tr("Totales del mes"), this);
    auto totalesTitleFont = totalesTitle->font();
    totalesTitleFont.setBold(true);
    totalesTitle->setFont(totalesTitleFont);
    layout->addWidget(totalesTitle);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);

    auto *usdHeader = new QLabel(QStringLiteral("USD"), this);
    auto *arsHeader = new QLabel(QStringLiteral("ARS"), this);
    auto *consHeader = new QLabel(tr("Consolidado"), this);
    usdHeader->setAlignment(Qt::AlignRight);
    arsHeader->setAlignment(Qt::AlignRight);
    consHeader->setAlignment(Qt::AlignRight);
    auto headerFont = usdHeader->font();
    headerFont.setBold(true);
    usdHeader->setFont(headerFont);
    arsHeader->setFont(headerFont);
    consHeader->setFont(headerFont);

    grid->addWidget(usdHeader, 0, 1);
    grid->addWidget(arsHeader, 0, 2);
    grid->addWidget(consHeader, 0, 3);

    grid->addWidget(new QLabel(tr("Actual"), this), 1, 0);
    m_actualUsdValue = makeValueLabel();
    m_actualArsValue = makeValueLabel();
    m_actualConsolidadoValue = makeValueLabel();
    grid->addWidget(m_actualUsdValue, 1, 1);
    grid->addWidget(m_actualArsValue, 1, 2);
    grid->addWidget(m_actualConsolidadoValue, 1, 3);

    grid->addWidget(new QLabel(tr("Gastado"), this), 2, 0);
    m_gastadoUsdValue = makeValueLabel();
    m_gastadoArsValue = makeValueLabel();
    m_gastadoConsolidadoValue = makeValueLabel();
    grid->addWidget(m_gastadoUsdValue, 2, 1);
    grid->addWidget(m_gastadoArsValue, 2, 2);
    grid->addWidget(m_gastadoConsolidadoValue, 2, 3);

    layout->addLayout(grid);

    m_consolidadoHint = new QLabel(
        tr("Ingresa el tipo de cambio del mes para ver el total consolidado en pesos."), this);
    m_consolidadoHint->setWordWrap(true);
    layout->addWidget(m_consolidadoHint);
}

QLabel *TotalesPanelWidget::makeValueLabel()
{
    auto *label = new QLabel(this);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

void TotalesPanelWidget::setTasa(std::optional<std::int64_t> usdAArsCentavos)
{
    if (usdAArsCentavos.has_value()) {
        m_tasaEdit->setText(formatMoney(*usdAArsCentavos));
    } else {
        m_tasaEdit->clear();
    }
}

void TotalesPanelWidget::setTotales(const TotalesMes &totales, std::optional<std::int64_t> usdAArsCentavos)
{
    styleMoneyValue(m_actualUsdValue, totales.actualUsd);
    styleMoneyValue(m_actualArsValue, totales.actualArs);
    styleMoneyValue(m_gastadoUsdValue, totales.gastadoUsd);
    styleMoneyValue(m_gastadoArsValue, totales.gastadoArs);

    const bool hasTasa = usdAArsCentavos.has_value();
    m_actualConsolidadoValue->setVisible(hasTasa);
    m_gastadoConsolidadoValue->setVisible(hasTasa);
    m_consolidadoHint->setVisible(!hasTasa);

    if (hasTasa) {
        styleMoneyValue(m_actualConsolidadoValue,
                        consolidadoEnArs(totales.actualArs, totales.actualUsd, *usdAArsCentavos));
        styleMoneyValue(m_gastadoConsolidadoValue,
                        consolidadoEnArs(totales.gastadoArs, totales.gastadoUsd, *usdAArsCentavos));
    } else {
        m_actualConsolidadoValue->clear();
        m_gastadoConsolidadoValue->clear();
    }
}

void TotalesPanelWidget::onGuardarTasa()
{
    const auto parsed = parseMoney(m_tasaEdit->text());
    if (!parsed.has_value() || *parsed <= 0) {
        QMessageBox::warning(this, tr("Datos invalidos"),
                             tr("Ingresa un tipo de cambio valido mayor a cero."));
        return;
    }

    emit tasaGuardada(*parsed);
}
