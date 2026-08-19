#pragma once

#include "model/Cuenta.h"
#include "model/Movimiento.h"

#include <optional>
#include <vector>

#include <QDate>
#include <QDialog>
#include <QStringList>

class QComboBox;
class QDateEdit;
class QLineEdit;

class MovimientoDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Tipo
    {
        Ingreso,
        Egreso
    };

    explicit MovimientoDialog(const std::vector<Cuenta> &cuentas, const QDate &fechaDefault,
                              const QStringList &categorias, QWidget *parent = nullptr);

    void setCuentaId(std::int64_t cuentaId);
    void setDatosEdicion(const Movimiento &movimiento);

    std::int64_t cuentaId() const;
    QDate fecha() const;
    Tipo tipo() const;
    std::int64_t montoCentavos() const;
    QString concepto() const;
    QString categoria() const;

private:
    void accept() override;

    QComboBox *m_cuentaCombo = nullptr;
    QComboBox *m_tipoCombo = nullptr;
    QComboBox *m_categoriaCombo = nullptr;
    QDateEdit *m_fechaEdit = nullptr;
    QLineEdit *m_montoEdit = nullptr;
    QLineEdit *m_conceptoEdit = nullptr;
};
