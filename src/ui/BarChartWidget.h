#pragma once

#include "model/GastoCategoria.h"

#include <QWidget>

class BarChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget *parent = nullptr);

    void setTitulo(const QString &titulo);
    void setDatos(const std::vector<GastoCategoria> &datos);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_titulo;
    std::vector<GastoCategoria> m_datos;
};
