#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "QMediaPlayer"
#include "QObject"
#include "QAudioOutput"
#include "QAudioBufferOutput"

class AudioPlayer: public QMediaPlayer
{
    Q_OBJECT

    public:
        bool _fadeOut = false;
        bool _fadeIn = false;
        bool isFading = false;

        float fadeFactor = 0.2f;
        float maxVolume = 1.0f;

        AudioPlayer();
        void Reset();
        void Play();
        void Stop();
        void addMedia(QString file);
        void Seek(int mseconds);
        qint64 getPosition();
        qint64 getDuration();
        qint64 remainingTime();
        bool isPlaying();
        bool isPaused();
        bool isStopped();
        qreal getVolume();
        void setVolume(float volume);
        void fadeOut();
        void fadeIn();
        void fade();
        void setBuffer(QAudioBufferOutput *output);

    signals:
        void update_position( qint64 position );

    private slots:
        void positionChanged(qint64 position );

    private:
        int current_length;





        QMediaPlayer *player;
        QAudioOutput *audioOutput;
};

#endif // AUDIOPLAYER_H
