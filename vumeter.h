#ifndef VUMETER_H
#define VUMETER_H

#include <QWidget>
#include <QColor>

class VuMeter : public QWidget
{
    Q_OBJECT

    public:
        explicit VuMeter(QWidget *parent = nullptr);

        void setLevel(int level);        // de 0 a 10
        void setColor(QColor color);     // cor dos LEDs

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        int level = 0;
        QColor ledColor = Qt::green;
};

#endif // VUMETER_H
