#include <QtWidgets>
#include <QMediaPlayer>
#include "audioplayer.h"
#include <QDebug>
#include <QTimer>
#include <QAudioOutput>
#include <QAudioBufferOutput>

AudioPlayer::AudioPlayer()
{
    current_length = 0;
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;

    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(0);

    //connect(player, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
    connect(player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);

    QTimer *fadeTimer = new QTimer(this);
    connect(fadeTimer, &QTimer::timeout, this, &AudioPlayer::fade);
    fadeTimer->start(500);
}

void AudioPlayer::Reset()
{
    player->stop();
    _fadeOut = false;
    _fadeIn = false;
    isFading = false;
    fadeFactor = 0.1f;
    maxVolume = 1.0f;
    current_length = 0;
}

void AudioPlayer::Play()
{
    player->play();
}

void AudioPlayer::Stop()
{
    player->stop();
}

void AudioPlayer::positionChanged( qint64 position )
{
    emit update_position( position );
}

void AudioPlayer::Seek(int mseconds)
{
    player->setPosition(mseconds);
}

void AudioPlayer::addMedia(QString file)
{
    player->setSource(QUrl::fromLocalFile( file ));
}

qint64 AudioPlayer::getDuration()
{
    return player->duration();
}

qint64 AudioPlayer::getPosition()
{
    return player->position();
}

qint64 AudioPlayer::remainingTime()
{
    qint64 remainingDuration = getDuration() - getPosition();
    return remainingDuration;
}

bool AudioPlayer::isPlaying()
{
    if(player->playbackState()==QMediaPlayer::PlayingState) return true;
    return false;
}

bool AudioPlayer::isPaused()
{
    if(player->playbackState()==QMediaPlayer::PausedState) return true;
    return false;
}

bool AudioPlayer::isStopped()
{
    if(player->playbackState()==QMediaPlayer::StoppedState) return true;
    return false;
}

qreal AudioPlayer::getVolume()
{
    return audioOutput->volume();
}
void AudioPlayer::setVolume(float volume)
{
    audioOutput->setVolume( volume );
}

void AudioPlayer::fadeOut()
{
    if(isPlaying()) _fadeOut = true;
}

void AudioPlayer::fadeIn()
{
    if(isStopped()) _fadeIn = true;
}

void AudioPlayer::fade()
{
    if(_fadeOut || (isPlaying() && getVolume()>maxVolume && _fadeIn==false )) { _fadeIn=false; isFading = true; setVolume( getVolume() - fadeFactor ); }
    if(_fadeIn || (isPlaying() && getVolume()<maxVolume && _fadeOut==false )) { _fadeOut=false; isFading = true; setVolume( getVolume() + fadeFactor ); }

    if(_fadeOut && getVolume()<=0 && isPlaying()) { Stop(); _fadeOut = false; isFading = false; }
    if(getVolume()>=maxVolume && isPlaying()) { _fadeIn = false; isFading = false; }

    if(getVolume()>0 && isStopped()) { _fadeIn = false; _fadeIn = true; isFading = false; maxVolume = 0; setVolume(0); }

    //qDebug() << "vol: " << getVolume() << " -- " << isFading << " -- " << isStopped();
}

void AudioPlayer::setBuffer(QAudioBufferOutput *output)
{
    player->setAudioBufferOutput(output);
}
