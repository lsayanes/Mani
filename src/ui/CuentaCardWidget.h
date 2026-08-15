#pragma once

#include "model/Cuenta.h"

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;

class CuentaCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CuentaCardWidget(const Cuenta &cuenta, QWidget *parent = nullptr);

    void setCuenta(const Cuenta &cuenta);
    std::int64_t cuentaId() const;

signals:
    void editRequested(std::int64_t cuentaId);
    void movimientosRequested(std::int64_t cuentaId);
    void deleteRequested(std::int64_t cuentaId);

private:
    void refresh();
    QHBoxLayout *addMoneyRow(QVBoxLayout *layout, const QString &prefix, QLabel **valueOut);

    Cuenta m_cuenta;
    QLabel *m_nombreLabel = nullptr;
    QLabel *m_inicialValue = nullptr;
    QLabel *m_actualValue = nullptr;
    QLabel *m_gastadoValue = nullptr;
};
