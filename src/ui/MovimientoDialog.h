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
class QFormLayout;
class QLabel;
class QLineEdit;

class MovimientoDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Tipo
    {
        Ingreso,
        Egreso,
        Transferencia
    };

    explicit MovimientoDialog(const std::vector<Cuenta> &cuentas, const QDate &fechaDefault,
                              const QStringList &categorias, QWidget *parent = nullptr);

    void setCuentaId(std::int64_t cuentaId);
    void setDatosEdicion(const Movimiento &movimiento);

    std::int64_t cuentaId() const;
    std::int64_t cuentaDestinoId() const;
    QDate fecha() const;
    Tipo tipo() const;
    std::int64_t montoCentavos() const;
    std::int64_t montoOrigenCentavos() const;
    std::int64_t montoDestinoCentavos() const;
    QString concepto() const;
    QString categoria() const;

private slots:
    void updateTransferUi();

private:
    void accept() override;
    const Cuenta *cuentaPorId(std::int64_t cuentaId) const;
    std::optional<std::int64_t> parseMontoAbsoluto(const QString &texto) const;

    std::vector<Cuenta> m_cuentas;
    bool m_editando = false;
    std::int64_t m_montoOriginal = 0;

    QFormLayout *m_form = nullptr;
    QComboBox *m_cuentaCombo = nullptr;
    QComboBox *m_destinoCombo = nullptr;
    QComboBox *m_tipoCombo = nullptr;
    QComboBox *m_categoriaCombo = nullptr;
    QDateEdit *m_fechaEdit = nullptr;
    QLineEdit *m_montoEdit = nullptr;
    QLineEdit *m_montoDestinoEdit = nullptr;
    QLineEdit *m_conceptoEdit = nullptr;
    QLabel *m_cuentaLabel = nullptr;
    QLabel *m_destinoLabel = nullptr;
    QLabel *m_montoLabel = nullptr;
    QLabel *m_montoDestinoLabel = nullptr;
};
