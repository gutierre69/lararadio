#ifndef BUTTONHOLE_H
#define BUTTONHOLE_H

#include "audioplayer.h"
#include <QWidget>
#include <QPushButton>
#include <QSettings>
#include <QKeyEvent>

class ButtonHole : public QWidget
{
    Q_OBJECT

    public:
        explicit ButtonHole(QWidget *parent = nullptr);

        void setPositon(int newX, int newY);
        void setBtnText(QString newText);

    protected:
        void keyPressEvent(QKeyEvent *event) override;

    public slots:
        void bntContextMenu(QPoint pos);
        void buttonHoleClick();
        void buttonHoleStop();
        void buttonHoleKeyPress();
        void flash();

    private:
        QPushButton *button;

        QString filename = "";
        QString text = "";
        int x = 0;
        int y = 0;
        int width = 60;
        int height = 40;

        QSettings *settings;

        QMediaPlayer *player;
        QAudioOutput *audioOutput;
};

#endif // BUTTONHOLE_H
