#pragma once

#include "model/Cuenta.h"

#include <QDialog>

class QComboBox;
class QLineEdit;

class CuentaDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Create,
        Edit
    };

    explicit CuentaDialog(Mode mode, QWidget *parent = nullptr);

    void setCuenta(const Cuenta &cuenta);

    QString nombre() const;
    Moneda moneda() const;
    std::int64_t saldoInicialCentavos() const;
    std::int64_t saldoActualCentavos() const;

private:
    void accept() override;

    Mode m_mode;
    QLineEdit *m_nombreEdit = nullptr;
    QComboBox *m_monedaCombo = nullptr;
    QLineEdit *m_saldoInicialEdit = nullptr;
    QLineEdit *m_saldoActualEdit = nullptr;
};
