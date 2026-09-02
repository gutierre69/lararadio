#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QTreeWidgetItem>
#include "ui_mainwindow.h"
#include "audioplayer.h"
#include "timerclock.h"
#include "vumeter.h"
#include "buttonhole.h"
#include "QAudioOutput"
#include "QFileSystemModel"
#include <QProgressBar>
#include <QAudioFormat>
#include <QAudioInput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include <QAudioBufferOutput>
#include <QAudioBuffer>
#include <QSettings>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QShowEvent>
#include <QTranslator>
#include <QPainter>
#include <QPaintEvent>
#include "version.h"

#include "taglib/tag.h"
#include "taglib/mpegfile.h"
#include "taglib/tbytevectorstream.h"
#include "taglib/id3v2tag.h"
#include "taglib/id3v2framefactory.h"
#include "taglib/fileref.h"


typedef struct Playlist
{
    QString name = "";
    QString path = "";
    QString duration = "";
    QString type = "";
} Playlist;

class QTimer;
class QProcess;
class QNetworkAccessManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;

public slots:
    void directoryViewer();
    void initConfig();
    void init();
    void exit();
    void next();
    void restoreVolumeAudio(QMediaPlayer::MediaStatus state);
    void updateAudioList(bool jump = false);
    void currentTimePosition(qint64 progress, int playerid);
    void currentTimePositionClock(QTime time);
    void updateClockLabel(QString text_time);
    void seek(int mseconds);
    void setVolumeSpeak(float volume);
    void saveConfig(QString file, QString field, QString value);
    void savePlaylist();
    void loadPlaylist();
    void loadRecentPlaylist();
    void saveAutosavePlaylist();
    void loadAutosavePlaylist();
    void insertAutoJingles();
    void clearPlaylist();
    void skipToNext();
    void checkAdvanceTrack();

private slots:
    void on_btn_play_clicked();
    void on_btn_stop_clicked();
    void on_btn_next_clicked();
    void on_audio_clock_clicked();
    void on_btn_add_time_item_clicked();
    void on_btn_remove_item_clicked();
    void on_btn_add_item_clicked();
    void on_btn_talk_clicked();
    void on_btn_stream_clicked();
    void onFilesItemDoubleClicked(const QModelIndex &index);
    void onJingleFilesItemDoubleClicked(const QModelIndex &index);
    void onPlaylistItemDoubleClicked(const QModelIndex &index);
    void updateDisplay();
    void flash();

    void changeLanguage(QString lang);
    void audioOptionsMenu(QPoint pos);
    void updateStreamButtonState();
    void updateStreamMetadata(const QString &overrideTitle = QString());

    void unSelectedJingle();
    void unSelectedFiles();

    void showAboutDialog();
    void showConfigDialog();
    void exportLog();
    void addLog(const QString &entry);
    void closeEvent(QCloseEvent *event) override;

private:
    QAudioFormat getAudioFormat();
    void calculateRMS(const QAudioBuffer &buffer);
    Playlist makePlaylistItem(const QString &filepath, const QString &type);
    void addDroppedUrls(const QList<QUrl> &urls, int insertRow);

    Ui::MainWindow *ui;
    AudioPlayer audioplayer1;
    AudioPlayer audioplayer2;
    QMediaPlayer *timeplayer;
    QAudioOutput *timeAudioOutput;
    // Preview (pré-escuta): player separado, toca os 15s iniciais de um item
    // da playlist num sink distinto do broadcast (audio/preview_device;
    // vazio = dispositivo padrão do sistema, ex. o fone).
    QMediaPlayer *previewPlayer = nullptr;
    QAudioOutput *previewOutput = nullptr;
    QTimer *previewTimer = nullptr;
    QString m_previewPath;
    std::vector<Playlist> playlist;

    QFileSystemModel *model;

    QTranslator* translator;

    TimerClock *clock;
    QString SayTimeAudio;

    QAudioBufferOutput* audioBufferOutput;

    QAudioDevice* inputDevice;
    QProgressBar* progressBar;
    QAudioSource* audioSource;
    QIODevice* audioInputDevice;
    int currentVU = 0;

    int current_play = 0;
    int next_play = 0;
    bool repeat = false;
    bool isPlaying = false;


    QSettings *settings;

    float volumeToTalk = 0.8f;

    int startTransitionAudioTime = 5;

    bool SayingTimer = false;
    bool Talking = false;

    QString time_audio_path = QDir::homePath();

    const QString config_path = QDir::homePath() + "/.config/LaraRadio";

    int currentVU_L = 0;
    int currentVU_R = 0;
    VuMeter *vuMeterL;
    VuMeter *vuMeterR;

    // ButtonHole *buttonHole;
    std::vector<ButtonHole*> buttonHole;
    ButtonHole *loopBh = nullptr;

    // Playback log (exported to a txt in the user's home via ConfigDialog).
    QStringList m_log;

    // Auto-jingle: insert N jingles every M played tracks (autojingle/*).
    int m_musicCount = 0;

    // Streaming (Icecast/Shoutcast): ffmpeg child capturando o sink broadcast.
    QProcess *m_streamProc = nullptr;
    bool m_streamOn = false;
    QNetworkAccessManager *m_net = nullptr;
    QTimer *m_metaRetryTimer = nullptr;

    bool m_uiReady = false;
    bool m_recentPlaylistLoaded = false;
    QTimer *m_displayTimer = nullptr;
    int m_silenceMs = 0;
    qint64 m_lastPos = 0;
};
#endif // MAINWINDOW_H
