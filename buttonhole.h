#ifndef BUTTONHOLE_H
#define BUTTONHOLE_H

#include "audioplayer.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QAudioBufferOutput>
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
        void setBtnKey(QString newKey);
        void setLoopMode(bool loop);
        // Route this ButtonHole's audio to a specific output device
        // (e.g. the "broadcast" sink). Null/default keeps the system default.
        void setOutputDevice(const QAudioDevice &device);
        // Feed this ButtonHole's PCM into the app VU meter pipeline
        // (same QAudioBufferOutput used by the playlist players).
        void setBuffer(QAudioBufferOutput *output);
        bool isPlaying();
        void stopPlayback();
        void fadeOut();
        // File currently assigned to this button (from buttonhole/btn_<key>).
        QString getFilename() const { return filename; }
        float maxVolume = 1.0f;

    signals:
        void triggered();

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
        QString btnKey = "";
        int x = 0;
        int y = 0;
        int width = 60;
        int height = 40;
        bool loopMode = false;
        bool m_fadeOut = false;

        QSettings *settings;

        QMediaPlayer *player;
        QAudioOutput *audioOutput;
};

#endif // BUTTONHOLE_H
