#include "vumeter.h"
#include <QPainter>
#include <QBrush>
#include <QPaintEvent>

VuMeter::VuMeter(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 10);
}

void VuMeter::setLevel(int newLevel)
{
    level = newLevel; // limita entre 0 e 10
    update(); // força a repintura
}

void VuMeter::setColor(QColor color)
{
    ledColor = color;
    update();
}

void VuMeter::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(ledColor);
    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);

    int spacing = 1;
    int ledWidth = 5;
    int ledHeight = 10;

    for (int i = 0; i < 33; ++i) {
        QRect ledRect(i * (ledWidth + spacing), 0, ledWidth, ledHeight);

        QColor green_on = QColor(0, 255, 0);
        QColor green_off = QColor(0, 60, 0);

        QColor red_on = QColor(255, 0, 0);
        QColor red_off = QColor(60, 0, 0);

        QColor yellow_on = QColor(255, 255, 0);
        QColor yellow_off = QColor(60, 60, 0);

        if(i<=20){
            if (i < level)
                painter.setBrush(green_on);
            else
                painter.setBrush(green_off); // LED apagado
        }

        if(i>20 && i<=28){
            if (i < level)
                painter.setBrush(yellow_on);
            else
                painter.setBrush(yellow_off); // LED apagado
        }

        if(i>28){
            if (i < level)
                painter.setBrush(red_on);
            else
                painter.setBrush(red_off); // LED apagado
        }

        painter.drawRoundedRect(ledRect, 0, 0);
    }
}
