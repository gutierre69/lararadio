#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioBufferOutput>

class AudioPlayer: public QObject
{
    Q_OBJECT

    public:
        bool _fadeOut = false;
        bool _fadeIn = false;
        bool isFading = false;

        float fadeFactor = 0.2f;
        float maxVolume = 1.0f;

        AudioPlayer();
        ~AudioPlayer();
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
        void setOutputDevice(const QAudioDevice &device);
        bool hasError() const { return m_hasError; }
        static bool isValidMediaFile(const QString &path);

    signals:
        void update_position(qint64 position);
        void playbackFinished();
        void mediaError(const QString &file, const QString &errorMsg);

    private slots:
        void positionChanged(qint64 position);
        void onPlayerError(QMediaPlayer::Error error, const QString &errorString);

    private:
        int current_length = 0;
        QMediaPlayer *player;
        QAudioOutput *audioOutput;
        QAudioBufferOutput *audioBufferOutput = nullptr;
        QString cleanFilePath;
        bool m_hasError = false;

        QString transcodeIfNeeded(const QString &file);
};

#endif // AUDIOPLAYER_H
