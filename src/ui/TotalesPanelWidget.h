#pragma once

#include "util/Totales.h"

#include <cstdint>
#include <memory>
#include <optional>

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class DolarHoy;

class TotalesPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TotalesPanelWidget(QWidget *parent = nullptr);

    void setTasa(std::optional<std::int64_t> usdAArsCentavos);
    void setTotales(const TotalesMes &totales, std::optional<std::int64_t> usdAArsCentavos);

signals:
    void tasaGuardada(std::int64_t usdAArsCentavos);

private slots:
    void onGuardarTasa();
    void onDolarHoyClicked();
    void onDolarHoyFetched(std::int64_t promedioCentavos);
    void onDolarHoyFailed(const QString &message);

private:
    QLabel *makeValueLabel();

    QLineEdit *m_tasaEdit = nullptr;
    QPushButton *m_dolarHoyButton = nullptr;
    std::unique_ptr<DolarHoy> m_dolarHoy;
    QLabel *m_consolidadoHint = nullptr;
    QLabel *m_actualUsdValue = nullptr;
    QLabel *m_actualArsValue = nullptr;
    QLabel *m_actualConsolidadoValue = nullptr;
    QLabel *m_gastadoUsdValue = nullptr;
    QLabel *m_gastadoArsValue = nullptr;
    QLabel *m_gastadoConsolidadoValue = nullptr;
};
