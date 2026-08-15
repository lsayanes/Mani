#include "ui/BarChartWidget.h"

#include "util/Money.h"

#include <QPainter>
#include <QPaintEvent>

namespace {

constexpr int kMargin = 12;
constexpr int kBarHeight = 22;
constexpr int kBarGap = 8;
constexpr int kLabelWidth = 120;

} // namespace

BarChartWidget::BarChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
}

void BarChartWidget::setTitulo(const QString &titulo)
{
    m_titulo = titulo;
    update();
}

void BarChartWidget::setDatos(const std::vector<GastoCategoria> &datos)
{
    m_datos = datos;
    const int height = kMargin * 2 + static_cast<int>(m_datos.size()) * (kBarHeight + kBarGap) + 20;
    setMinimumHeight(qMax(120, height));
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().window());

    const QRect area = rect().adjusted(kMargin, kMargin, -kMargin, -kMargin);

    if (!m_titulo.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(palette().windowText().color());
        painter.drawText(area.x(), area.y() + 14, m_titulo);
    }

    if (m_datos.empty()) {
        painter.setFont(font());
        painter.setPen(palette().mid().color());
        painter.drawText(area, Qt::AlignCenter, tr("Sin egresos para mostrar"));
        return;
    }

    std::int64_t maxValor = 1;
    for (const GastoCategoria &dato : m_datos) {
        maxValor = qMax(maxValor, dato.totalCentavos);
    }

    int y = area.y() + (m_titulo.isEmpty() ? 0 : 24);
    const int barAreaWidth = area.width() - kLabelWidth - 100;

    QFont labelFont = painter.font();
    painter.setFont(labelFont);

    for (const GastoCategoria &dato : m_datos) {
        const int barWidth =
            static_cast<int>((static_cast<double>(dato.totalCentavos) / static_cast<double>(maxValor)) * barAreaWidth);

        painter.setPen(palette().windowText().color());
        const QString label = painter.fontMetrics().elidedText(dato.categoria, Qt::ElideRight, kLabelWidth - 4);
        painter.drawText(area.x(), y + kBarHeight - 6, kLabelWidth, kBarHeight, Qt::AlignVCenter | Qt::AlignLeft,
                         label);

        const QRect barRect(area.x() + kLabelWidth, y, qMax(barWidth, 2), kBarHeight);
        painter.fillRect(barRect, QColor(QStringLiteral("#27ae60")));

        painter.drawText(barRect.right() + 8, y, 90, kBarHeight, Qt::AlignVCenter | Qt::AlignLeft,
                         formatMoney(dato.totalCentavos));

        y += kBarHeight + kBarGap;
    }
}
