#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "about_dialog.h"
#include "configdialog.h"
#include "custonIconProvider.h"
#include "playlisttree.h"
#include <QMessageBox>
#include "QFileSystemModel"
#include "QTreeWidget"
#include "QAudioOutput"
#include <iostream>
#include <QTreeWidgetItem>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <cmath>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioBufferOutput>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAudioBuffer>
#include <QSlider>
#include <QIcon>
#include <QApplication>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QDesktopServices>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QFileInfo>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <algorithm>
#include <functional>


// Audio extensions accepted in playlist drag & drop (same as ButtonHole dialog).
static const QStringList kValidAudioSuffixes = {"mp3", "wav", "ogg", "flac", "mp4"};

static bool isAudioFile(const QFileInfo &fi)
{
    return fi.isFile() && kValidAudioSuffixes.contains(fi.suffix().toLower());
}

// Accept the drag if any URL is an audio file or a folder (folders expand on drop).
static bool hasDroppableAudio(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        const QFileInfo fi(url.toLocalFile());
        if (fi.isDir() || isAudioFile(fi)) return true;
    }
    return false;
}


MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size().width(), this->size().height());
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    //this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    //this->setAttribute(Qt::WA_TranslucentBackground);


    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = screenGeometry.center().x() - this->frameGeometry().width() / 2;
        int y = screenGeometry.center().y() - this->frameGeometry().height() / 2;
        this->move(x, y);
    }

    settings = new QSettings("LaraRadio", "LaraRadio", this);

    initConfig();
    init();
}

MainWindow::~MainWindow()
{
    audioplayer1.Stop();
    audioplayer2.Stop();

    delete timeplayer;
    delete timeAudioOutput;
    delete translator;
    delete ui;
}
void MainWindow::exit()
{
    this->close();
}

void MainWindow::initConfig()
{

    for(int i=1; i<=10; i++){
        if(!settings->contains("buttonhole/btn_"+QString::number(i))) settings->setValue("buttonhole/btn_"+QString::number(i), "");
    }
    if(!settings->contains("volume/volumeToTalk")) settings->setValue("volume/volumeToTalk", volumeToTalk);

    if(!settings->contains("volume/TransitionAudioTime")) settings->setValue("volume/TransitionAudioTime", startTransitionAudioTime);
    if(!settings->contains("volume/speedFade")) settings->setValue("volume/speedFade", audioplayer1.fadeFactor * 10);

    if(!settings->contains("volume/sayClock")) settings->setValue("volume/sayClock", true);
    if(!settings->contains("volume/sayClockFade")) settings->setValue("volume/sayClockFade", true);
    if(!settings->contains("volume/stopFade")) settings->setValue("volume/stopFade", true);
    if(!settings->contains("volume/talkFade")) settings->setValue("volume/talkFade", true);

    if(!settings->contains("files/defaultDir")) settings->setValue("files/defaultDir", QDir::homePath());
    if(!settings->contains("files/jingleDir")) settings->setValue("files/jingleDir", QDir::homePath());
    if(!settings->contains("files/audioTimeDir")) settings->setValue("files/audioTimeDir", QDir::homePath());

    if(!settings->contains("interface/language")) settings->setValue("interface/language", "en_US");
}

void MainWindow::init()
{
    m_uiReady = false;

    translator = new QTranslator(this);
    qApp->removeTranslator(translator);
    if(translator->load(":/languages/"+settings->value("interface/language", "en_US").toString()+".qm")){
        qApp->installTranslator(translator);
        ui->retranslateUi(this);
    }

    volumeToTalk = settings->value("volume/volumeToTalk", volumeToTalk).toFloat();
    startTransitionAudioTime = settings->value("volume/TransitionAudioTime", volumeToTalk).toInt();
    float factor = (float)(settings->value("volume/speedFade", audioplayer1.fadeFactor).toFloat() / 10);

    time_audio_path = settings->value("files/audioTimeDir").toString();

    audioplayer1.fadeFactor = factor;
    audioplayer2.fadeFactor = factor;



    timeplayer = new QMediaPlayer(this);
    timeAudioOutput = new QAudioOutput(this);

    timeplayer->setAudioOutput(timeAudioOutput);
    timeAudioOutput->setVolume(1.0f);

    // Preview (pré-escuta): player separado, toca os 15s iniciais de um item
    // da playlist. Sink: audio/preview_device (vazio = padrão do sistema).
    previewPlayer = new QMediaPlayer(this);
    previewOutput = new QAudioOutput(this);
    const QString previewDevId = settings->value("audio/preview_device").toString();
    if (!previewDevId.isEmpty()) {
        for (const QAudioDevice &d : QMediaDevices::audioOutputs()) {
            if (d.id() == previewDevId) {
                previewOutput->setDevice(d);
                break;
            }
        }
    }
    previewOutput->setVolume(1.0);
    previewPlayer->setAudioOutput(previewOutput);
    previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    connect(previewTimer, &QTimer::timeout, this, [=]() {
        previewPlayer->stop();
        m_previewPath.clear();
    });

    // Sink de saída principal (audio/output_device): ex. "broadcast" para
    // mandar a programação ao Icecast via ffmpeg. Vazio = padrão do sistema.
    const QString mainDev = settings->value("audio/output_device").toString();
    QAudioDevice mainOutputDevice;
    if (!mainDev.isEmpty()) {
        for (const QAudioDevice &d : QMediaDevices::audioOutputs()) {
            if (d.id().compare(mainDev.toUtf8(), Qt::CaseInsensitive) == 0
                || d.description().compare(mainDev, Qt::CaseInsensitive) == 0) {
                mainOutputDevice = d;
                audioplayer1.setOutputDevice(d);
                audioplayer2.setOutputDevice(d);
                timeAudioOutput->setDevice(d);
            }
        }
    }
    qDebug() << "ROUTE-DEBUG mainDev=" << mainDev
             << "| audioOutputs=" << QMediaDevices::audioOutputs().size()
             << "| mainOutputDevice null=" << mainOutputDevice.isNull();

    // Botão de stream desabilitado até haver config válida (stream/*).
    updateStreamButtonState();

    m_net = new QNetworkAccessManager(this);
    m_metaRetryTimer = new QTimer(this);
    m_metaRetryTimer->setSingleShot(true);
    connect(m_metaRetryTimer, &QTimer::timeout, this, [=]() { updateStreamMetadata(); });

    model = new QFileSystemModel(this);
    model->setRootPath( QDir::homePath() );
    model->setIconProvider(new CustomIconProvider);

    ui->files->setModel(model);
    ui->files->setRootIndex(model->index( settings->value("files/defaultDir", QDir::homePath()).toString() ));

    ui->jingle_files->setModel(model);
    ui->jingle_files->setRootIndex(model->index( settings->value("files/jingleDir", QDir::homePath()).toString() ));

    ui->audio_list->header()->resizeSection(0,30);
    ui->audio_list->header()->resizeSection(1,350);

    // audio level meter
    audioBufferOutput = new QAudioBufferOutput(this);
    audioplayer1.setBuffer(audioBufferOutput);
    audioplayer2.setBuffer(audioBufferOutput);

    connect(audioBufferOutput, &QAudioBufferOutput::audioBufferReceived, this, &MainWindow::calculateRMS);
    connect(timeplayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::restoreVolumeAudio);
    connect(timeplayer, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error, const QString &errorString) {
        qWarning() << "Timeplayer error:" << errorString;
        if (SayingTimer) {
            SayingTimer = false;
            current_play = (current_play + 1) % playlist.size();
            next();
        }
    });
    //connect(ui->files, &QTreeView::doubleClicked, this, &MainWindow::onFilesItemDoubleClicked);
    //connect(ui->jingle_files, &QTreeView::doubleClicked, this, &MainWindow::onJingleFilesItemDoubleClicked);
    connect(ui->audio_list, &QTreeWidget::doubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);

    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);

    QTimer *fadeTimer = new QTimer(this);
    connect(fadeTimer, &QTimer::timeout, this, &MainWindow::flash);
    fadeTimer->start(500);

    // clock
    clock = new TimerClock();
    connect(clock,SIGNAL(updateTime(QString)),this,SLOT(updateClockLabel(QString)));
    connect(clock, &TimerClock::updateSeparateTime,this, &MainWindow::currentTimePositionClock);

    connect(ui->seeker, &QSlider::sliderMoved, this, &MainWindow::seek);
    connect(ui->volume_speak, &QSlider::sliderMoved, this, &MainWindow::setVolumeSpeak);
    connect(&audioplayer1, &AudioPlayer::update_position, this, [=](qint64 position) {
        currentTimePosition(position, 1);
    });
    connect(&audioplayer2, &AudioPlayer::update_position, this, [=](qint64 position) {
        currentTimePosition(position, 2);
    });
    connect(&audioplayer1, &AudioPlayer::mediaError, this, [=](const QString &file, const QString &err) {
        qWarning() << "Player1 error on" << file << ":" << err;
        if (isPlaying) skipToNext();
    });
    connect(&audioplayer2, &AudioPlayer::mediaError, this, [=](const QString &file, const QString &err) {
        qWarning() << "Player2 error on" << file << ":" << err;
        if (isPlaying) skipToNext();
    });
    connect(&audioplayer1, &AudioPlayer::playbackFinished, this, [=]() {
        if (isPlaying) checkAdvanceTrack();
    });
    connect(&audioplayer2, &AudioPlayer::playbackFinished, this, [=]() {
        if (isPlaying) checkAdvanceTrack();
    });

    ui->version->setText( tr("Versão: ") + QString(APP_VERSION) );
    ui->volume_speak->setValue( volumeToTalk * 100 );

    connect(ui->files, &QTreeView::clicked, this, &MainWindow::unSelectedJingle);
    connect(ui->jingle_files, &QTreeView::clicked, this, &MainWindow::unSelectedFiles);

    ui->audio_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->audio_list, &QTreeWidget::customContextMenuRequested, this, &MainWindow::audioOptionsMenu); // use new syntax for more joy

    directoryViewer();

    connect(ui->menu_about_lara, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(ui->actionReleases, &QAction::triggered, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://lararadio.com/category/releases"));
    });
    connect(ui->actionLibs, &QAction::triggered, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://lararadio.com/technologies"));
    });
    connect(ui->actionTutorial, &QAction::triggered, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://lararadio.com/configuration-guide"));
    });
    connect(ui->actionContribute, &QAction::triggered, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://lararadio.com/contribute"));
    });

    connect(ui->actionConfig, &QAction::triggered, this, &MainWindow::showConfigDialog);
    connect(ui->actionSair, &QAction::triggered, this, &MainWindow::exit);
    connect(ui->actionSavePlaylist, &QAction::triggered, this, &MainWindow::savePlaylist);
    connect(ui->actionLoadPlaylist, &QAction::triggered, this, &MainWindow::loadPlaylist);
    connect(ui->actionClearAudioList, &QAction::triggered, this, &MainWindow::clearPlaylist);

    connect(ui->actionLanguagePTBR, &QAction::triggered, this, [=]() { changeLanguage("pt_BR"); });
    connect(ui->actionLanguageENUS, &QAction::triggered, this, [=]() { changeLanguage("en_US"); });

    vuMeterL = new VuMeter(this);
    vuMeterR = new VuMeter(this);
    vuMeterL->setGeometry(143, 146, 200, 10);
    vuMeterR->setGeometry(143, 170, 200, 10);
    vuMeterL->show();
    vuMeterR->show();

    for(int bi=1; bi<=10; bi++){
        ButtonHole *bh = new ButtonHole(this);
        bh->setGeometry(340 + ((bi-1)*70), 574, 60, 40);
        bh->setBtnText(QString::number( bi ));
        bh->setBtnKey(QString::number( bi ));
        if (!mainOutputDevice.isNull()) bh->setOutputDevice(mainOutputDevice);
        bh->show();

        buttonHole.push_back( bh );
    }

    // Loop button: a ButtonHole that keeps replaying its assigned audio.
    // Left of the "Botoeira" label (window is 1048x622 — below is clipped).
    loopBh = new ButtonHole(this);
    loopBh->setGeometry(180, 574, 60, 40);
    loopBh->setBtnText(tr("Loop"));
    loopBh->setBtnKey("Loop");
    loopBh->setLoopMode(true);
    if (!mainOutputDevice.isNull()) loopBh->setOutputDevice(mainOutputDevice);
    loopBh->setBuffer(audioBufferOutput); // feed VU meter on loop playback
    loopBh->show();

    // Priority between loop and playlist: when the loop is pressed while the
    // playlist is playing, fade the playlist out and let the loop take over.
    connect(loopBh, &ButtonHole::triggered, this, [=]() {
        addLog(tr("LOOP acionado (fade out da playlist)"));
        if (audioplayer1.isPlaying()) audioplayer1.fadeOut();
        if (audioplayer2.isPlaying()) audioplayer2.fadeOut();
        // "No ar": o loop assumiu o programa — mostra o arquivo dele na UI
        // e no Icecast (a playlist segue em fadeOut até o próximo play).
        const QString loopName = QFileInfo(loopBh->getFilename()).fileName();
        if (!loopName.isEmpty()) {
            ui->current_audio->setText("<p align='center'>" + loopName + "</p>");
            updateStreamMetadata(loopName);
        }
    });

    connect(ui->chk_repeat, &QCheckBox::toggled, this, [=](bool checked) { repeat = checked; });

    // Drag & drop into the playlist (external drops only).
    // Internal reorder is handled 100% manually in eventFilter() — Qt's
    // built-in drag (InternalMove OR DragDrop mode) always messes with the
    // tree visuals (ghost item, hiding, stacking). We disable it and use our
    // own QDrag carrying the source row in the mimeData.
    // PlaylistTree subclass handles ALL drag & drop itself (manual QDrag with
    // the source row in the mimeData) — it only emits what it wants done.
    connect(ui->audio_list, &PlaylistTree::reorderRequested, this, [=](int fromRow, int toRow) {
        if (fromRow < 0 || fromRow >= (int)playlist.size()) return;
        Playlist item = playlist[fromRow];
        playlist.erase(playlist.begin() + fromRow);
        if (toRow > fromRow) toRow--;
        playlist.insert(playlist.begin() + toRow, item);

        // Keep current_play pointing at the same track.
        if (current_play == fromRow) {
            current_play = toRow;
        } else if (current_play > fromRow && current_play <= toRow) {
            current_play--;
        } else if (current_play < fromRow && current_play >= toRow) {
            current_play++;
        }
        QTimer::singleShot(0, this, [=]() { updateAudioList(); });
    });
    connect(ui->audio_list, &PlaylistTree::urlsDropped, this, [=](const QList<QUrl> &urls, int row) {
        addDroppedUrls(urls, row);
    });

    m_uiReady = true;
    if (m_displayTimer) {
        m_displayTimer->start(10);
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (m_recentPlaylistLoaded) {
        return;
    }

    m_recentPlaylistLoaded = true;
    if (settings->value("autosave/enabled").toBool()) {
        QTimer::singleShot(0, this, &MainWindow::loadAutosavePlaylist);
    } else {
        QTimer::singleShot(0, this, &MainWindow::loadRecentPlaylist);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event){
    int key = event->key();

    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        int index = key - Qt::Key_1;
        if (key == Qt::Key_0) index = 9;

        if (index >= 0 && index < buttonHole.size()) {
            buttonHole.at(index)->buttonHoleKeyPress();
        }
    }
}


void MainWindow::changeLanguage(QString lang)
{
    qApp->removeTranslator(translator);
    if(translator->load(":/languages/"+lang+".qm")){
        qApp->installTranslator(translator);
        ui->retranslateUi(this);

        settings->setValue("interface/language", lang);
    }
}

void MainWindow::clearPlaylist()
{
    audioplayer1.Reset();
    audioplayer2.Reset();
    isPlaying = false;
    current_play = 0;
    next_play = 0;
    playlist.clear();
    updateAudioList();
}

void MainWindow::audioOptionsMenu(QPoint pos)
{
    QTreeWidgetItem *item = ui->audio_list->itemAt(pos);
    if (!item)
        return;

    QModelIndex index = ui->audio_list->indexFromItem(item);

    QMenu *menu = new QMenu(this);

    QAction* markHasNext = new QAction(QIcon(":/images/icons/go-last.svg"), tr("Marcar como Próximo"), this);
    QAction* playThis = new QAction(QIcon(":/images/icons/media-playback-start.svg"), tr("Tocar Este"), this);
    QAction* monitorThis = new QAction(QIcon(":/images/icons/preferences-desktop-sound.svg"), tr("Pré Escuta"), this);
    QAction* deleteThis = new QAction(QIcon(":/images/icons/edit-delete.svg"), tr("Apagar"), this);
    menu->addAction( playThis );
    menu->addAction( markHasNext );
    menu->addAction( monitorThis );
    menu->addSeparator();
    menu->addAction( deleteThis );
    menu->popup(ui->audio_list->viewport()->mapToGlobal(pos));

    connect(playThis, &QAction::triggered, this, [=]() {
        onPlaylistItemDoubleClicked(index);
        ui->audio_list->clearSelection();
    });

    // Pré-escuta: toca os 15s iniciais do item no preview player (sink
    // próprio). Clique de novo no mesmo item = para a pré-escuta.
    connect(monitorThis, &QAction::triggered, this, [=]() {
        const int row = index.row();
        if (row < 0 || row >= (int)playlist.size())
            return;
        const QString &path = playlist[row].path;
        if (m_previewPath == path && previewPlayer->playbackState() == QMediaPlayer::PlayingState) {
            previewPlayer->stop();
            m_previewPath.clear();
            ui->audio_list->clearSelection();
            return;
        }
        m_previewPath = path;
        previewPlayer->setSource(QUrl::fromLocalFile(path));
        previewPlayer->play();
        previewTimer->start(15000);
        ui->audio_list->clearSelection();
    });

    connect(deleteThis, &QAction::triggered, this, &MainWindow::on_btn_remove_item_clicked);

    connect(markHasNext, &QAction::triggered, this, [=]() {
        // Move the clicked track to right after the currently playing one, so
        // it actually plays next (next_play = current_play+1, label included).
        int from = index.row();
        if (from < 0 || from >= (int)playlist.size() || from == current_play)
            return;

        Playlist item = playlist[from];
        playlist.erase(playlist.begin() + from);

        // Erasing before current_play shifts current_play left by one.
        if (from < current_play) current_play--;
        int insertPos = current_play + 1;
        playlist.insert(playlist.begin() + insertPos, item);

        // next_play is recomputed by updateAudioList() (jump=false default),
        // which also refreshes the "Próxima" label.
        updateAudioList();
        ui->audio_list->clearSelection();
    });
}

void MainWindow::unSelectedJingle()
{
    ui->jingle_files->selectionModel()->clearSelection();
    ui->jingle_files->selectionModel()->clearCurrentIndex();
}
void MainWindow::unSelectedFiles()
{
    ui->files->selectionModel()->clearSelection();
    ui->files->selectionModel()->clearCurrentIndex();
}

void MainWindow::seek(int mseconds)
{
    if(audioplayer1.isPlaying()) audioplayer1.Seek(mseconds);
    if(audioplayer2.isPlaying()) audioplayer2.Seek(mseconds);
}

void MainWindow::updateClockLabel(QString text_time)
{
    ui->audio_clock->setText(text_time);

    SayTimeAudio = text_time;
    SayTimeAudio = SayTimeAudio.remove(":");

    QString hour_str = SayTimeAudio.left(2);

        /// adjust time for not 24 hours
        int hour = hour_str.toInt();
            if(hour>12) hour = hour - 12;
            hour_str = QString::number(hour);

        if(hour<10)
            SayTimeAudio = "0"+hour_str+SayTimeAudio.right(4);
        else
            SayTimeAudio = hour_str+SayTimeAudio.right(4);

    SayTimeAudio = SayTimeAudio.left(4);
}

void MainWindow::on_btn_remove_item_clicked()
{
    // Remove every selected row (multi-selection), highest row first so
    // earlier indexes stay valid while erasing.
    QList<int> rows;
    const auto items = ui->audio_list->selectedItems();
    for (QTreeWidgetItem *item : items)
        rows << ui->audio_list->indexOfTopLevelItem(item);
    if (rows.isEmpty())
        return;

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        if (row < 0 || row >= (int)playlist.size()) continue;
        playlist.erase(playlist.begin() + row);
    }

    // Keep current_play/next_play valid after the playlist shrank
    if (current_play >= (int)playlist.size()) current_play = playlist.size() - 1;
    if (next_play >= (int)playlist.size()) next_play = playlist.size() - 1;
    if (current_play < 0) current_play = 0;
    if (next_play < 0) next_play = 0;
    updateAudioList();
}

void MainWindow::on_btn_add_item_clicked()
{
    int index = ui->audio_list->currentIndex().row();
    QModelIndex file_index = ui->files->currentIndex();
    QModelIndex jungle_index = ui->jingle_files->currentIndex();

    if((file_index.row()>=0 || jungle_index.row()>=0)){
        QString filepath = "";
        QString type = "";

        if (file_index.row()>=0 && file_index.isValid()) {
            filepath = model->filePath(file_index);
            if(!model->isDir(file_index)) type = "music"; else type = "folder-music";
        }

        if (jungle_index.row()>=0 && jungle_index.isValid()) {
            filepath = model->filePath(jungle_index);
            if(!model->isDir(jungle_index)) type = "jingle"; else type = "folder-jingle";
        }

        if(filepath=="")
            return;

        Playlist item = makePlaylistItem(filepath, type);

        // Always append at the END of the playlist (not after the currently
        // selected/playing row) — matches how the listener expects new songs
        // to queue up after everything already added.
        playlist.push_back(item);
        updateAudioList();
    }
}

Playlist MainWindow::makePlaylistItem(const QString &filepath, const QString &type)
{
    Playlist item;
    item.path = filepath;
    item.type = type;
    item.name = QFileInfo(filepath).completeBaseName();
    item.duration = "--:--";

    // Folders are resolved at play time (random pick) — no tags to read.
    if (type != "folder-music" && type != "folder-jingle") {
        TagLib::FileRef aud(filepath.toStdString().c_str());
        if (!aud.isNull() && aud.audioProperties()) {
            int totalSeconds = aud.audioProperties()->length();
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;

            item.duration = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));

            if(aud.tag()->artist()!="" && aud.tag()->title()!="") {
                item.name = QString::fromStdString( aud.tag()->title().toCString(true) ) + " - " + QString::fromStdString( aud.tag()->artist().toCString(true) );
            }
        }
    }
    return item;
}

void MainWindow::addDroppedUrls(const QList<QUrl> &urls, int insertRow)
{
    QStringList files;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        const QFileInfo fi(url.toLocalFile());
        if (fi.isDir()) {
            // Folder drop: expand every audio file inside, one after another.
            // QDir name filters need glob patterns ("*.wav"), bare suffixes match nothing.
            QStringList filters;
            for (const QString &s : kValidAudioSuffixes) filters << "*." + s;
            QDirIterator it(fi.absoluteFilePath(), filters,
                            QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
            while (it.hasNext()) files << it.next();
        } else if (isAudioFile(fi)) {
            files << fi.absoluteFilePath();
        }
    }
    if (files.isEmpty()) return;

    files.sort(); // deterministic order for "one after another"

    int row = insertRow;
    for (const QString &file : std::as_const(files)) {
        if (row >= 0) {
            playlist.insert(playlist.begin() + (row + 1), makePlaylistItem(file, "music"));
            row++;
        } else {
            playlist.push_back(makePlaylistItem(file, "music"));
        }
    }
    updateAudioList();
}

void MainWindow::on_btn_talk_clicked()
{
    if(Talking==false) {
        Talking=true;
        ui->btn_talk->setStyleSheet("background-color: red;");
        addLog(tr("TALK ON (locução ativada)"));
    } else {
        Talking=false;
        ui->btn_talk->setStyleSheet("");
        addLog(tr("TALK OFF (locução desativada)"));
    }

    // ON AIR light: same mechanism as the old standalone button, now tied to
    // the TALK (locution) state. Fires the configured script with the matching
    // parameter (e.g. usbrelay2 with "on1"/"off1"). Skips if path doesn't exist.
    // Config: onair/script + onair/param_on + onair/param_off (ConfigDialog).
    QString script = settings->value("onair/script").toString();
    if (!script.isEmpty() && QFile::exists(script)) {
        QString param = Talking ? settings->value("onair/param_on", "on1").toString()
                                : settings->value("onair/param_off", "off1").toString();
        qDebug() << "ON AIR" << (Talking ? "ON" : "OFF") << "->" << script << param;
        QProcess::startDetached(script, QStringList() << param);
    }
}

// O botão de stream só fica clicável com config válida (URL + usuário +
// senha preenchidos). Com o stream LIGADO continua clicável (pra desligar),
// mesmo que a config tenha sido zerada depois.
void MainWindow::updateStreamButtonState()
{
    const bool valid =
        !settings->value("stream/url").toString().trimmed().isEmpty()
        && !settings->value("stream/user").toString().trimmed().isEmpty()
        && !settings->value("stream/pass").toString().isEmpty();
    ui->btn_stream->setEnabled(valid || m_streamOn);
    qDebug() << "[stream] valid=" << valid
             << "url=" << settings->value("stream/url").toString()
             << "user=" << settings->value("stream/user").toString()
             << "enabled=" << ui->btn_stream->isEnabled();
}

// Atualiza o "curr playing" (título) do mount no Icecast via /admin/metadata.
// O source autentica com as próprias credenciais. Só Icecast — Shoutcast tem
// protocolo próprio de metadata, não suportado aqui.
// Retry: logo após ligar o stream o ffmpeg ainda não conectou (mount não
// existe) e o Icecast rejeita — se o HTTP não for 2xx, re-agenda em 2s.
void MainWindow::updateStreamMetadata(const QString &overrideTitle)
{
    if (!m_streamOn || !m_net) return;
    // Já tem um retry pendente? Esta chamada é o retry (timer singleShot já
    // disparou quando chega aqui) — mas se veio de um play novo enquanto o
    // retry está agendado, deixa o retry pendente cuidar (lê a música atual).
    if (m_metaRetryTimer->isActive()) return;
    const QString url = settings->value("stream/url").toString().trimmed();
    if (url.isEmpty() || url.contains("shoutcast://")) return;
    // overrideTitle: o que está no ar fora da playlist (ex: loop button).
    // Vazio = título da música atual da playlist.
    QString song = overrideTitle;
    if (song.isEmpty()) {
        if (current_play < 0 || current_play >= (int)playlist.size()) return;
        song = playlist[current_play].name;
    }
    if (song.isEmpty()) return;

    // host:porta + mount a partir da URL (ex: localhost:9000/live → /live).
    QString host = url;
    if (host.contains("://")) host = host.mid(host.indexOf("://") + 3);
    QString mount = "/";
    const int slash = host.indexOf('/');
    if (slash >= 0) { mount = host.mid(slash); host = host.left(slash); }
    if (!mount.startsWith('/')) mount.prepend('/');

    QUrl reqUrl(QString("http://%1/admin/metadata").arg(host));
    QUrlQuery q;
    q.addQueryItem("mount", mount);
    q.addQueryItem("mode", "updinfo");
    q.addQueryItem("song", song);
    reqUrl.setQuery(q);

    QNetworkRequest req(reqUrl);
    const QString user = settings->value("stream/user").toString().trimmed();
    const QString pass = settings->value("stream/pass").toString();
    req.setRawHeader("Authorization",
                     "Basic " + QString(user + ":" + pass).toUtf8().toBase64());
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if ((status < 200 || status >= 300) && m_streamOn)
            m_metaRetryTimer->start(2000);
        reply->deleteLater();
    });
}

// Stream ON/OFF (Icecast/Shoutcast): dispara/para um ffmpeg que captura o
// monitor do sink de broadcast e envia pro servidor configurado (stream/*).
// A URL do ConfigDialog aceita protocolo explícito ou assume Icecast.
void MainWindow::on_btn_stream_clicked()
{
    if (m_streamOn) {
        // Desliga o stream. Desvincula o member e manda o SIGKILL — o signal
        // finished dispara no event loop e o handler (que usa sender(), nunca
        // o member) limpa o processo. Sem waitForFinished: bloquear aqui
        // reentraria o finished com m_streamProc já nulo.
        QProcess *p = m_streamProc;
        m_streamProc = nullptr;
        m_streamOn = false;
        ui->btn_stream->setChecked(false);
        ui->btn_stream->setStyleSheet("");
        if (p) p->kill();
        addLog(tr("STREAM OFF"));
        return;
    }

    const QString url = settings->value("stream/url").toString().trimmed();
    const QString user = settings->value("stream/user").toString().trimmed();
    const QString pass = settings->value("stream/pass").toString();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Streaming"),
                             tr("Configure a URL do servidor (Configurações → Streaming) antes de ligar o stream."));
        ui->btn_stream->setChecked(false);
        return;
    }

    // Mata ffmpeg ÓRFÃO de sessão anterior: app morto com kill -9 não mata o
    // filho, que continua segurando o mount ("Mountpoint in use" no Icecast).
    QFile pidFile(QDir::tempPath() + "/lararadio_stream.pid");
    if (pidFile.exists()) {
        const qint64 oldPid = pidFile.readAll().trimmed().toLongLong();
        if (oldPid > 0) {
            QProcess *killer = new QProcess(this);
            killer->start("kill", {"-9", QString::number(oldPid)});
            connect(killer, &QProcess::finished, killer, &QObject::deleteLater);
        }
        pidFile.remove();
    }

    // Monta a URL de source: mantém o protocolo explícito, injeta user:pass@
    // logo após o //. Sem protocolo, assume Icecast.
    QString target;
    if (url.contains("://")) {
        target = url.mid(0, url.indexOf("://") + 3) + user + ":" + pass + "@" + url.mid(url.indexOf("://") + 3);
    } else {
        target = "icecast://" + user + ":" + pass + "@" + url;
    }

    // Fonte de áudio: monitor do sink de broadcast (configurável por máquina).
    const QString source = settings->value("stream/source", "broadcast.monitor").toString();

    m_streamProc = new QProcess(this);
    m_streamProc->setProgram("ffmpeg");
    QStringList args = {
        "-hide_banner", "-loglevel", "error",
        "-f", "pulse", "-i", source,
        "-c:a", "libmp3lame", "-b:a", "128k",
        "-f", "mp3",
        // -user_agent é opção do PROTOCOLO http do output — tem que vir
        // depois do -f e antes da URL (em posição global o ffmpeg 6.1
        // responde "Option user_agent not found" e o stream morre no start).
        "-user_agent", "LaraRadio",
    };
    // Identificação do stream no Icecast (ice_name/ice_description são
    // opções do protocolo icecast, mesma posição de output).
    const QString streamName = settings->value("stream/name").toString().trimmed();
    const QString streamDesc = settings->value("stream/description").toString().trimmed();
    if (!streamName.isEmpty()) args << "-ice_name" << streamName;
    if (!streamDesc.isEmpty()) args << "-ice_description" << streamDesc;
    args << target;
    m_streamProc->setArguments(args);
    m_streamProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_streamProc, &QProcess::finished, this, [=](int code, QProcess::ExitStatus) {
        // NUNCA usar m_streamProc aqui: no desligamento manual o member já é
        // nullptr quando o finished roda. Tudo via sender().
        QProcess *p = qobject_cast<QProcess *>(sender());
        const QByteArray out = p ? p->readAllStandardOutput() : QByteArray();
        if (!out.isEmpty()) qWarning() << "[ffmpeg stream]" << out.constData();
        // Caiu sozinho (desligamento manual já zerou m_streamOn antes do kill):
        // desliga o botão e avisa.
        if (m_streamOn) {
            addLog(tr("STREAM caiu (ffmpeg exit %1)").arg(code));
            m_streamOn = false;
            ui->btn_stream->setChecked(false);
            ui->btn_stream->setStyleSheet("");
        }
        if (p) {
            // Limpa o lock se este era o processo registrado.
            QFile pidFile(QDir::tempPath() + "/lararadio_stream.pid");
            if (pidFile.exists() && pidFile.readAll().trimmed().toLongLong() == p->processId())
                pidFile.remove();
            p->deleteLater();
        }
    });
    m_streamProc->start();
    // Registra o PID no lock pra limpar órfãos na próxima sessão.
    QFile pidOut(QDir::tempPath() + "/lararadio_stream.pid");
    if (pidOut.open(QIODevice::WriteOnly)) {
        pidOut.write(QString::number(m_streamProc->processId()).toUtf8());
        pidOut.close();
    }

    m_streamOn = true;
    ui->btn_stream->setChecked(true);
    ui->btn_stream->setStyleSheet("background-color: #0a7d0a;");
    addLog(tr("STREAM ON (%1)").arg(target));
    updateStreamMetadata();
}

void MainWindow::setVolumeSpeak(float volume)
{
    volumeToTalk = volume / 100;
    settings->setValue("volume/volumeToTalk", QString::number( volumeToTalk));
}

void MainWindow::on_audio_clock_clicked()
{
    QString say_audio = time_audio_path+"/"+SayTimeAudio+".mp3";

    timeplayer->setSource(QUrl::fromLocalFile(say_audio));

    if(
        audioplayer1.isFading == false
        && audioplayer2.isFading == false
    ){
        if(audioplayer1.isPlaying()) audioplayer1.setVolume(0.5);
        if(audioplayer2.isPlaying()) audioplayer2.setVolume(0.5);
    }

    timeAudioOutput->setVolume(1);
    timeplayer->play();
}

void MainWindow::currentTimePosition(qint64 progress, int playerid)
{
    if(!isPlaying)
        return;

        qint64 totalDuration = 0;
        qint64 currentPosition = 0;
        qint64 remainingDuration = 0;

        if(audioplayer1.isPlaying() && audioplayer1.maxVolume>0){
            remainingDuration = audioplayer1.remainingTime();

            ui->seeker->setMaximum( audioplayer1.getDuration() );
            ui->seeker->setValue( audioplayer1.getPosition() );
        }
        if(audioplayer2.isPlaying() && audioplayer2.maxVolume>0){
            remainingDuration = audioplayer2.remainingTime();

            ui->seeker->setMaximum( audioplayer2.getDuration() );
            ui->seeker->setValue( audioplayer2.getPosition() );
        }

        qint64 seconds = (remainingDuration / 1000) % 60;
        qint64 minutes = (remainingDuration / 1000) / 60;

        if(
            minutes==0
            && seconds==startTransitionAudioTime
            && !audioplayer1.isFading
            && !audioplayer2.isFading
            && (playlist[current_play].type=="music" || playlist[current_play].type=="folder-music")
            && (repeat || current_play < (int)playlist.size() - 1)
        ) {
            current_play += 1;
            if(current_play>(playlist.size()-1)) current_play = 0;
            next();

            if(audioplayer1.isPlaying()) audioplayer1.isFading=true;
            if(audioplayer2.isPlaying()) audioplayer2.isFading=true;
        }

        // update remain time
        QString formattedTime = QString("<p align='center'>%1:%2</p>").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
        ui->remain_time->setText(formattedTime);

}
void MainWindow::currentTimePositionClock(QTime time)
{
    if (!isPlaying)
        return;

        qint64 remainingDuration = 0;

        if(audioplayer1.isPlaying() && audioplayer1.maxVolume>0)
            remainingDuration = audioplayer1.remainingTime();

        if(audioplayer2.isPlaying() && audioplayer2.maxVolume>0)
            remainingDuration = audioplayer2.remainingTime();

        QTime endTime = time.addSecs(remainingDuration / 1000);

        // update remain time clock
        QString formattedTime = QString("<p align='center'>%1</p>").arg(endTime.toString("HH:mm:ss"));
        ui->over_at_time->setText(formattedTime);

}

void MainWindow::onFilesItemDoubleClicked(const QModelIndex &index)
{
    if (index.isValid() && !model->isDir(index)) {
        QVariant data = index.model()->data(index, Qt::DisplayRole);
        QString filename = data.toString();

        filename = filename.remove(".mp3");
        filename = filename.remove(".wav");
        filename = filename.remove(".flac");
        filename = filename.remove(".ogg");

        QString filepath = model->filePath(index);

        QString duration = "";
        TagLib::FileRef aud(filepath.toStdString().c_str());
        if (!aud.isNull() && aud.audioProperties()) {
            int totalSeconds = aud.audioProperties()->length();
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;

            duration = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));

            if(aud.tag()->artist()!="" && aud.tag()->title()!="") {
                filename = QString::fromStdString( aud.tag()->title().toCString(true) ) + " - " + QString::fromStdString( aud.tag()->artist().toCString(true) );
            }
        }

        playlist.push_back({filename, filepath, duration, "music"});
        updateAudioList();
    }
}

void MainWindow::onJingleFilesItemDoubleClicked(const QModelIndex &index)
{
    if (index.isValid() && !model->isDir(index)) {
        QVariant data = index.model()->data(index, Qt::DisplayRole);
        QString filename = data.toString();

        filename = filename.remove(".mp3");
        filename = filename.remove(".wav");
        filename = filename.remove(".flac");
        filename = filename.remove(".ogg");

        QString filepath = model->filePath(index);

        QString duration = "";
        TagLib::FileRef aud(filepath.toStdString().c_str());
        if (!aud.isNull() && aud.audioProperties()) {
            int totalSeconds = aud.audioProperties()->length();
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;

            duration = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));

            if(aud.tag()->artist()!="" && aud.tag()->title()!="") {
                filename = QString::fromStdString( aud.tag()->title().toCString(true) ) + " - " + QString::fromStdString( aud.tag()->artist().toCString(true) );
            }
        }

        playlist.push_back({filename, filepath, duration, "jingle"});
        updateAudioList();
    }
}

void MainWindow::onPlaylistItemDoubleClicked(const QModelIndex &index)
{
    current_play = index.row();
    isPlaying = true;
    next();
}

void MainWindow::directoryViewer()
{
    model->setHeaderData(0, Qt::Vertical, tr("Nome"));
    ui->files->setModel(model);
    ui->files->hideColumn(1);
    ui->files->hideColumn(2);
    ui->files->hideColumn(3);

    ui->jingle_files->setModel(model);
    ui->jingle_files->hideColumn(1);
    ui->jingle_files->hideColumn(2);
    ui->jingle_files->hideColumn(3);
}

void MainWindow::on_btn_play_clicked()
{
    if(playlist.size()==0)
        return;
    next();
}

void MainWindow::on_btn_stop_clicked()
{
    if(playlist.size()==0 && !audioplayer1.isPlaying() && !audioplayer2.isPlaying())
        return;

    audioplayer1.Reset();
    audioplayer2.Reset();

    isPlaying = false;

    updateAudioList();

    current_play = 0;
    next_play = 0;

    currentVU_L = 0;
    currentVU_R = 0;
    vuMeterL->setLevel(currentVU_L);
    vuMeterR->setLevel(currentVU_R);
}

void MainWindow::on_btn_next_clicked()
{
    if(playlist.size()==0)
        return;

    if(!audioplayer1.isPlaying() && !audioplayer2.isPlaying())
        return;

    if(audioplayer1.isFading==false && audioplayer2.isFading==false){
        current_play = next_play;
        if(current_play>(playlist.size()-1)) current_play = 0;
        next();
    }
}

void MainWindow::restoreVolumeAudio(QMediaPlayer::MediaStatus state)
{
    if(state==QMediaPlayer::EndOfMedia && isPlaying){

        if(
            audioplayer1.isFading == false
            && audioplayer2.isFading == false
        ){
            if(audioplayer1.isPlaying()) audioplayer1.setVolume(audioplayer1.maxVolume); else audioplayer1.setVolume(0);
            if(audioplayer2.isPlaying()) audioplayer2.setVolume(audioplayer2.maxVolume); else audioplayer2.setVolume(0);
        }

        if(SayingTimer){
            SayingTimer = false;
            current_play += 1;
            if(current_play>(playlist.size()-1)) current_play = 0;
            next();
        }
    }
}

void MainWindow::skipToNext()
{
    if (playlist.size() == 0) return;

    audioplayer1.Reset();
    audioplayer2.Reset();

    current_play = (current_play + 1) % playlist.size();
    next();
}

void MainWindow::checkAdvanceTrack()
{
    if (!isPlaying || playlist.size() == 0) return;
    if (audioplayer1.isStopped() && audioplayer2.isStopped() && !SayingTimer) {
        // Last track and repeat disabled: stop instead of looping forever.
        if (!repeat && current_play >= (int)playlist.size() - 1) {
            isPlaying = false;
            return;
        }
        current_play = (current_play + 1) % playlist.size();
        next();
    }
}

void MainWindow::next()
{
    if(playlist.size()>0){
        if (current_play >= (int)playlist.size()) current_play = 0;
        if (current_play < 0) current_play = 0;
        if(SayingTimer==false){
            isPlaying = true;

            // Playlist takes priority over the loop: fade the loop out
            // (not an abrupt cut) and let the new track fade in.
            if (loopBh && loopBh->isPlaying()) loopBh->fadeOut();

            if(audioplayer1.isPlaying()) audioplayer1.fadeOut();
            if(audioplayer2.isPlaying()) audioplayer2.fadeOut();

            QString type = playlist[ current_play ].type;
            QString path = playlist[ current_play ].path;

            if(type=="folder-music" || type=="folder-jingle"){

                QDir audioDir( path );
                QStringList filters;
                filters << "*.mp3" << "*.wav" << "*.ogg" << "*.flac" << "*.mp4"; // Add other formats as needed
                QStringList audioFiles = audioDir.entryList(filters, QDir::Files);


                if (!audioFiles.isEmpty()) {
                    int randomIndex = QRandomGenerator::global()->bounded(audioFiles.size());
                    QString selectedFile = audioFiles.at(randomIndex);
                    path = audioDir.absoluteFilePath(selectedFile);

                    if(type=="folder-music")  type = "music";
                    if(type=="folder-jingle")  type = "jingle";


                }


            }

            if(type=="music"){
                addLog(tr("PLAY [musica] %1").arg(QFileInfo(path).fileName()));
                updateStreamMetadata();

                // Auto-jingle: count played tracks; every N tracks, insert M
                // random jingles from the jingle dir right after this one.
                m_musicCount++;
                if (settings->value("autojingle/enabled").toBool()) {
                    const int interval = settings->value("autojingle/interval", 5).toInt();
                    if (m_musicCount >= interval) {
                        m_musicCount = 0;
                        insertAutoJingles();
                    }
                }
                if(audioplayer2.isPlaying()){
                    audioplayer1.addMedia( path );
                    audioplayer1.Play();
                    audioplayer1.fadeIn();

                } else if(
                    (audioplayer1.isPlaying())
                    || (
                        audioplayer1.isStopped()
                        && audioplayer2.isStopped()
                        )
                    ){
                    audioplayer2.addMedia( path );
                    audioplayer2.Play();
                    audioplayer2.fadeIn();
                }
            }

            if(type=="jingle"){
                addLog(tr("PLAY [vinheta] %1").arg(QFileInfo(path).fileName()));
                updateStreamMetadata();
                if(audioplayer2.isPlaying()){
                    audioplayer1.addMedia( path );
                    audioplayer1.maxVolume = 1.0f;
                    audioplayer1.setVolume( audioplayer1.maxVolume );
                    audioplayer1.Play();

                } else if(
                    (audioplayer1.isPlaying())
                    || (
                        audioplayer1.isStopped()
                        && audioplayer2.isStopped()
                        )
                    ){
                    audioplayer2.addMedia( path );
                    audioplayer2.maxVolume = 1.0f;
                    audioplayer2.setVolume( audioplayer2.maxVolume );
                    audioplayer2.Play();
                }
            }
        }

        if(playlist[ current_play ].type=="time" && SayingTimer==false){
            QString say_audio = time_audio_path+"/"+SayTimeAudio+".mp3";

            if (QFile::exists(say_audio)) {
                timeplayer->setSource(QUrl::fromLocalFile(say_audio));
                timeAudioOutput->setVolume(1);
                timeplayer->play();
                SayingTimer=true;
            } else {
                qWarning() << "Time audio not found:" << say_audio << "- skipping time item";
                // File doesn't exist: don't set SayingTimer, don't advance.
                // The flash() watchdog will see both players stopped with
                // isPlaying && !SayingTimer and advance naturally.
            }
        }

    } else {
        std::cout << "nothing" << std::endl;
    }
    updateAudioList();
}

void MainWindow::updateAudioList(bool jump)
{
    int index = ui->audio_list->currentIndex().row();
    ui->audio_list->clear();

    // Guard: playlist can be empty (e.g. cleared/removed while playing)
    if (!playlist.empty() && current_play >= 0 && current_play < (int)playlist.size())
        ui->current_audio->setText( "<p align='center'>"+playlist[ current_play ].name+"</p>" );
    else
        ui->current_audio->setText( "<p align='center'></p>" );

    if(!playlist.empty() && playlist.size()>current_play && jump==false){
        next_play = current_play + 1;
        if(next_play>(playlist.size()-1)) next_play = 0;

        ui->next_audio->setText( "<p align='center'>"+playlist[ next_play ].name+"</p>" );
    }

    for(auto& playlist_item : playlist){
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->audio_list);

        if(playlist_item.type=="music")
            item->setIcon(0, QIcon(":/images/icons/audio-x-generic.png"));

        if(playlist_item.type=="jingle")
            item->setIcon(0, QIcon(":/images/icons/audio-x-mpegurl.png"));

        if(playlist_item.type=="time")
            item->setIcon(0, QIcon(":/images/icons/clock.svg"));

        if(playlist_item.type=="folder-music" || playlist_item.type=="folder-jingle")
            item->setIcon(0, QIcon(":/images/icons/folder.png"));

        item->setText(1, playlist_item.type=="time"?tr(playlist_item.name.toLocal8Bit()):playlist_item.name);
        item->setText(2, playlist_item.duration);

        for(int i=0; i<3; i++){
            QBrush bgcolor = QBrush(QColor(0, 0, 0));
            if(playlist_item.type=="music") bgcolor = QBrush(QColor(2, 28, 0));
            if(playlist_item.type=="jingle") bgcolor = QBrush(QColor(23, 12, 0));
            if(playlist_item.type=="time") bgcolor = QBrush(QColor(23, 23, 23));

            item->setBackground(i,bgcolor);
            item->setForeground(i,Qt::white);

        }
    }

    if(isPlaying){
        // Guard: current_play/next_play can go stale if the playlist was
        // edited while playing — topLevelItem() returns nullptr for an
        // invalid row, and dereferencing it is a segfault.
        QTreeWidgetItem *curItem = (current_play >= 0 && current_play < playlist.size())
            ? ui->audio_list->topLevelItem(current_play) : nullptr;
        QTreeWidgetItem *nextItem = (next_play >= 0 && next_play < playlist.size())
            ? ui->audio_list->topLevelItem(next_play) : nullptr;

        for(int i=0; i<3; i++){
            if (curItem) {
                curItem->setBackground(i,QBrush(QColor(5, 223, 114)));
                curItem->setForeground(i,Qt::black);
            }
            if (nextItem && next_play!=current_play){
                nextItem->setForeground(i,Qt::white);
                nextItem->setBackground(i,QBrush(QColor(255, 100, 103)));
            }
        }
    }

    //if(index!=current_play && index!=next_play && isPlaying)
        ui->audio_list->setCurrentItem( ui->audio_list->topLevelItem(index) );
}

void MainWindow::on_btn_add_time_item_clicked()
{
    playlist.push_back({tr("Hora Certa"), "", "--:--",  "time"});
    updateAudioList();
}

void MainWindow::updateDisplay() {
    if (!m_uiReady || !ui || !vuMeterL || !vuMeterR) {
        return;
    }

    vuMeterL->setLevel(currentVU_L);
    vuMeterR->setLevel(currentVU_R);

    // Silence / audio failure watchdog
    // A player in PlayingState whose position does NOT advance for
    // SILENCE_TIMEOUT ms is treated as a failure and skipped.
    // (VU meter alone is unreliable: the FFmpeg backend does not feed
    //  QAudioBufferOutput, so levels stay at 0 even while audio plays.)
    if (isPlaying) {
        const int SILENCE_TIMEOUT = 10000; // 10 seconds
        AudioPlayer *active = nullptr;
        if (audioplayer1.isPlaying()) active = &audioplayer1;
        else if (audioplayer2.isPlaying()) active = &audioplayer2;

        if (active && !active->isFading) {
            qint64 pos = active->getPosition();
            bool advancing = (pos > m_lastPos);
            m_lastPos = pos;
            if (advancing) {
                m_silenceMs = 0;
            } else {
                m_silenceMs += 10; // displayTimer interval
                if (m_silenceMs >= SILENCE_TIMEOUT) {
                    qWarning() << "Silence watchdog: position stalled for" << SILENCE_TIMEOUT
                               << "ms — skipping track";
                    m_silenceMs = 0;
                    skipToNext();
                }
            }
        } else {
            m_silenceMs = 0;
        }
    } else {
        m_silenceMs = 0;
    }
}

void MainWindow::flash()
{
    if(Talking && audioplayer1.isPlaying()){
        audioplayer1.maxVolume = volumeToTalk;
    } else if(Talking==false && audioplayer1.isPlaying()){
        audioplayer1.maxVolume = 1.0f;
    }

    if(Talking && audioplayer2.isPlaying()){
        audioplayer2.maxVolume = volumeToTalk;
    } else if(Talking==false && audioplayer2.isPlaying()){
        audioplayer2.maxVolume = 1.0f;
    }

    // TALK ducks the loop too (ButtonHole::flash drives its volume to maxVolume)
    if (loopBh) {
        loopBh->maxVolume = Talking ? volumeToTalk : 1.0f;
    }

    if(Talking){
        if(ui->btn_talk->styleSheet()!=""){
            ui->btn_talk->setStyleSheet("");
        } else {
            ui->btn_talk->setStyleSheet("background-color: red;");
        }
    }

    if(audioplayer1.isFading || audioplayer2.isFading){
        if(ui->btn_next->styleSheet()!=""){
            ui->btn_next->setStyleSheet("");
        } else {
            ui->btn_next->setStyleSheet("background-color: black;");
        }
    }

    if(audioplayer1.isFading==false && audioplayer2.isFading==false){
        ui->btn_next->setStyleSheet("");
    }

    // "NO AR" lights up red while anything is on air: the playlist
    // (isPlaying) or the loop button (which takes over the program).
    if(isPlaying || (loopBh && loopBh->isPlaying())){
        ui->groupBox->setStyleSheet("QGroupBox:title {background-color: red;}");
    } else {
        ui->groupBox->setStyleSheet("");
    }
}

void MainWindow::calculateRMS(const QAudioBuffer &buffer)
{
    const int channels = buffer.format().channelCount();
    if (channels < 1 || channels > 2)
        return;

    const void *raw = buffer.constData<void>();
    const int samples = buffer.sampleCount();

    double sumL = 0.0, sumR = 0.0;

    /* ---------- 16‑bit signed -------------------- */
    if (buffer.format().sampleFormat() == QAudioFormat::Int16) {
        const qint16 *s = static_cast<const qint16 *>(raw);
        for (int i = 0; i < samples; i += channels) {
            double l = static_cast<double>(s[i]) / 32768.0;
            sumL += l * l;

            if (channels == 2) {
                double r = static_cast<double>(s[i + 1]) / 32768.0;
                sumR += r * r;
            }
        }
    }
    /* ---------- 32‑bit float --------------------- */
    else if (buffer.format().sampleFormat() == QAudioFormat::Float) {
        const float *s = static_cast<const float *>(raw);
        for (int i = 0; i < samples; i += channels) {
            double l = static_cast<double>(s[i]);
            sumL += l * l;

            if (channels == 2) {
                double r = static_cast<double>(s[i + 1]);
                sumR += r * r;
            }
        }
    } else {
        return;
    }

    int frames = samples / channels;
    double rmsL = std::sqrt(sumL / frames);
    double rmsR = (channels == 2) ? std::sqrt(sumR / frames) : rmsL;

    auto toVU = [](double rms) -> int {
        if (rms <= 0.0)
            return 0;
        double db = 20.0 * std::log10(rms);
        /* map -60 dB → 0    0 dB → 33 */
         return std::clamp(static_cast<int>(33.0 * (db + 60.0) / 60.0), 0, 33);
    };

    currentVU_L = toVU(rmsL);
    currentVU_R = toVU(rmsR);
}



void MainWindow::saveConfig(QString file, QString field, QString value)
{

    settings->setValue(field, value);
}

void MainWindow::showAboutDialog()
{
    AboutDialog aboutDialog;
    aboutDialog.setWindowTitle(tr("Sobre o LaraRadio"));

    QRect parentRect = this->geometry();
    int x = parentRect.center().x() - aboutDialog.width() / 2;
    int y = parentRect.center().y() - aboutDialog.height() / 2;
    aboutDialog.move(x, y);

    aboutDialog.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (settings->value("autosave/enabled").toBool()) {
        saveAutosavePlaylist();
    }
    if (previewPlayer) previewPlayer->stop();
    if (m_streamProc) m_streamProc->kill();
    QMainWindow::closeEvent(event);
}

void MainWindow::addLog(const QString &entry)
{
    m_log.append(QString("[%1] %2").arg(QTime::currentTime().toString("HH:mm:ss"), entry));
    if (m_log.size() > 2000) m_log.removeFirst();
}

void MainWindow::exportLog()
{
    const QString path = QDir::homePath() + "/LaraRadio_playback_report.txt";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Exportar Log"), tr("Não foi possível criar:\n%1").arg(path));
        return;
    }
    QTextStream out(&f);
    out << "LaraRadio — Log / Playback Report\n";
    out << "Gerado em: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "Máquina: " << QSysInfo::machineHostName() << "\n";
    out << "----------------------------------------\n";
    for (const QString &line : m_log) out << line << "\n";
    f.close();
    QMessageBox::information(this, tr("Exportar Log"),
        tr("Log exportado com sucesso para:\n%1\n\n(%2 linhas)").arg(path).arg(m_log.size()));
}

void MainWindow::showConfigDialog()
{
    ConfigDialog configDialog;
    configDialog.setWindowTitle(tr("Configurar"));
    connect(&configDialog, &ConfigDialog::exportLogRequested, this, &MainWindow::exportLog);

    QRect parentRect = this->geometry();
    int x = parentRect.center().x() - configDialog.width() / 2;
    int y = parentRect.center().y() - configDialog.height() / 2;
    configDialog.move(x, y);

    configDialog.exec();

    // Autosave: when the option is (or was just) enabled, persist the
    // current settings + playlist immediately — not only on app close.
    if (settings->value("autosave/enabled").toBool()) {
        saveAutosavePlaylist();
    }

    // Stream config may have changed: enable/disable the stream button.
    updateStreamButtonState();
}

void MainWindow::savePlaylist()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Salvar Arquivo"), QDir::homePath()+"/playlist.txt", tr("Arquivos de Texto")+" (*.txt);;"+tr("Todos os arquivos")+" (*.*)");

    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for(auto& playlist_item : playlist){
                out << playlist_item.name << "|" << playlist_item.path << "|" << playlist_item.duration << "|" << playlist_item.type << "\n";
            }
            file.close();
            QMessageBox::information(this, tr("Sucesso"), tr("Sua playlist foi salva o com sucesso!"));

            settings->setValue("files/recent", filename);
        } else {
            QMessageBox::critical(this, tr("Erro"), tr("Não foi possível salvar a playlist."));
        }
    }
}

void MainWindow::loadRecentPlaylist()
{
    QString recentFile = settings->value("files/recent").toString();
    if(recentFile!=""){
        QFile file(recentFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            playlist.clear();

            QTextStream in(&file);
            while (!in.atEnd()) {
                QString row = in.readLine();
                QStringList column = row.split('|');
                if (column.size() == 4) {
                    QString filename = column[0].trimmed();
                    QString filepath = column[1].trimmed();
                    QString duration = column[2].trimmed();
                    QString type = column[3].trimmed();

                    playlist.push_back({filename, filepath, duration, type});
                }
            }
            updateAudioList();
            file.close();
        } else {
            QMessageBox::critical(this, tr("Erro"), tr("Não foi possível carregar a playlist."));
        }
    }
}

void MainWindow::saveAutosavePlaylist()
{
    const QString path = QDir::homePath() + "/LaraRadio_autosave.txt";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Autosave: could not write" << path;
        return;
    }
    QTextStream out(&file);
    for (const auto &item : playlist) {
        out << item.name << "|" << item.path << "|" << item.duration << "|" << item.type << "\n";
    }
    file.close();
    qInfo() << "Autosave: playlist salva em" << path;
}

void MainWindow::loadAutosavePlaylist()
{
    const QString path = QDir::homePath() + "/LaraRadio_autosave.txt";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Autosave: no playlist to load at" << path;
        return;
    }
    playlist.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList column = in.readLine().split('|');
        if (column.size() == 4) {
            playlist.push_back({column[0].trimmed(), column[1].trimmed(),
                                column[2].trimmed(), column[3].trimmed()});
        }
    }
    file.close();
    updateAudioList();
    qInfo() << "Autosave: playlist carregada de" << path << "(" << playlist.size() << "itens)";
}

// Auto-jingle: pick `count` random audio files from the jingle dir and
// insert them as jingle items right after the currently playing track.
// Only inserts what exists — a missing/empty jingle dir is silently skipped.
void MainWindow::insertAutoJingles()
{
    const int count = settings->value("autojingle/count", 1).toInt();
    const QString jingleDir = settings->value("files/jingleDir", QDir::homePath()).toString();

    QStringList filters;
    for (const QString &s : kValidAudioSuffixes) filters << "*." + s;
    const QStringList files = QDir(jingleDir).entryList(filters, QDir::Files, QDir::Name);
    if (files.isEmpty()) return;

    // Insert immediately after the current track; shift right as we add.
    int insertRow = current_play + 1;
    for (int i = 0; i < count; ++i) {
        const QString file = files.at(QRandomGenerator::global()->bounded(files.size()));
        const QString fullPath = QDir(jingleDir).absoluteFilePath(file);
        Playlist item = makePlaylistItem(fullPath, "jingle");
        playlist.insert(playlist.begin() + insertRow++, item);
        addLog(tr("VINHETA automatica inserida: %1").arg(QFileInfo(fullPath).fileName()));
    }
    updateAudioList();
}

void MainWindow::loadPlaylist()
{
    if(isPlaying){
        QMessageBox::information(this, tr("Opss"), tr("Não é possivel carregar estando NO AR."));
        return;
    }
    QString filename = QFileDialog::getOpenFileName(this, tr("Carregar Playlist"), QDir::homePath(), tr("Arquivos de Texto")+" (*.txt);;"+tr("Todos os arquivos")+" (*.*)");

    if (!filename.isEmpty()) {

        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            playlist.clear();


            QTextStream in(&file);
            while (!in.atEnd()) {
                QString row = in.readLine();
                QStringList column = row.split('|');
                if (column.size() == 4) {
                    QString filename = column[0].trimmed();
                    QString filepath = column[1].trimmed();
                    QString duration = column[2].trimmed();
                    QString type = column[3].trimmed();

                    playlist.push_back({filename, filepath, duration, type});
                }
            }
            updateAudioList();
            file.close();

            on_btn_stop_clicked();

            ui->audio_list->clearSelection();
            ui->audio_list->clearFocus();
            ui->audio_list->selectionModel()->clearCurrentIndex();

            settings->setValue("files/recent", filename);
        } else {
            QMessageBox::critical(this, tr("Erro"), tr("Não foi possível carregar a playlist."));
        }
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    // QPainter p(this);
    // p.setRenderHint(QPainter::Antialiasing);

    // QBrush brush(QColor("#2d283c"));
    // p.setBrush(brush);
    // p.setPen(Qt::NoPen);

    // QRect rect = this->rect();
    // p.drawRoundedRect(rect, 15, 15);
}
