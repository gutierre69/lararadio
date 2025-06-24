#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "about_dialog.h"
#include "configdialog.h"
#include "custonIconProvider.h"
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
#include <QAudioBuffer>
#include <QSlider>
#include <QIcon>
#include <QApplication>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QDesktopServices>


MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size().width(), this->size().height());
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

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
    //connect(ui->files, &QTreeView::doubleClicked, this, &MainWindow::onFilesItemDoubleClicked);
    //connect(ui->jingle_files, &QTreeView::doubleClicked, this, &MainWindow::onJingleFilesItemDoubleClicked);
    connect(ui->audio_list, &QTreeWidget::doubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    timer->start(10);

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

    ui->version->setText( tr("Versão: ") + QString(APP_VERSION) );
    ui->volume_speak->setValue( volumeToTalk * 100 );

    connect(ui->files, &QTreeView::clicked, this, &MainWindow::unSelectedJingle);
    connect(ui->jingle_files, &QTreeView::clicked, this, &MainWindow::unSelectedFiles);

    ui->audio_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->audio_list, &QTreeWidget::customContextMenuRequested, this, &MainWindow::audioOptionsMenu); // use new syntax for more joy

    directoryViewer();
    loadRecentPlaylist();

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
        bh->show();

        buttonHole.push_back( bh );
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

    QAction* markHasNext = new QAction(QIcon(":/images/icons/go-last.svg"), "Marcar como Próximo", this);
    QAction* playThis = new QAction(QIcon(":/images/icons/media-playback-start.svg"), "Tocar Este", this);
    QAction* monitorThis = new QAction(QIcon(":/images/icons/preferences-desktop-sound.svg"), "Pré Escuta", this);
    QAction* deleteThis = new QAction(QIcon(":/images/icons/edit-delete.svg"), "Apagar", this);
    menu->addAction( playThis );
    menu->addAction( markHasNext );
    menu->addAction( monitorThis );
    menu->addSeparator();
    menu->addAction( deleteThis );
    menu->popup(ui->audio_list->viewport()->mapToGlobal(pos));

    monitorThis->setEnabled(false);

    connect(playThis, &QAction::triggered, this, [=]() {
        onPlaylistItemDoubleClicked(index);
        ui->audio_list->clearSelection();
    });

    connect(deleteThis, &QAction::triggered, this, &MainWindow::on_btn_remove_item_clicked);

    connect(markHasNext, &QAction::triggered, this, [=]() {
        next_play = index.row();
        updateAudioList(true);
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
    int row = ui->audio_list->currentIndex().row();
    playlist.erase( playlist.begin() + row);
    updateAudioList();
}

void MainWindow::on_btn_add_item_clicked()
{
    int index = ui->audio_list->currentIndex().row();
    QModelIndex file_index = ui->files->currentIndex();
    QModelIndex jungle_index = ui->jingle_files->currentIndex();

    QString filepath = "";
    QString filename = "";
    QString type = "";
    QString duration = "--:--";

    if((file_index.row()>=0 || jungle_index.row()>=0)){
        if (file_index.row()>=0 && file_index.isValid()) {
            QVariant data = file_index.model()->data(file_index, Qt::DisplayRole);
            filename = data.toString();
            filepath = model->filePath(file_index);
            if(!model->isDir(file_index)) type = "music"; else type = "folder-music";
        }

        if (jungle_index.row()>=0 && jungle_index.isValid()) {
            QVariant data = jungle_index.model()->data(jungle_index, Qt::DisplayRole);
            filename = data.toString();
            filepath = model->filePath(jungle_index);
            if(!model->isDir(jungle_index)) type = "jingle"; else type = "folder-jingle";
        }

        if(filename=="")
            return;

        filename = filename.remove(".mp3");
        filename = filename.remove(".wav");
        filename = filename.remove(".flac");
        filename = filename.remove(".ogg");


        if(type!="folder"){
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
        }

        if(index>=0){
            playlist.insert( playlist.begin() + (index+1), {filename, filepath, duration, type});
        } else {
            playlist.push_back({filename, filepath, duration, type});
        }
        updateAudioList();
    }
}

void MainWindow::on_btn_talk_clicked()
{
    if(Talking==false) {
        Talking=true;
        ui->btn_talk->setStyleSheet("background-color: red;");
    } else {
        Talking=false;
        ui->btn_talk->setStyleSheet("");
    }
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
    if(playlist.size()==0)
        return;

    if(!audioplayer1.isPlaying() && !audioplayer2.isPlaying())
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

void MainWindow::next()
{
    if(playlist.size()>0){
        if(SayingTimer==false){
            isPlaying = true;

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

            timeplayer->setSource(QUrl::fromLocalFile(say_audio));
            timeAudioOutput->setVolume(1);
            timeplayer->play();
            SayingTimer=true;
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

    ui->current_audio->setText( "<p align='center'>"+playlist[ current_play ].name+"</p>" );

    if(playlist.size()>current_play && jump==false){
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
        for(int i=0; i<3; i++){
            ui->audio_list->topLevelItem(current_play)->setBackground(i,QBrush(QColor(5, 223, 114)));
            ui->audio_list->topLevelItem(current_play)->setForeground(i,Qt::black);

            if(next_play!=current_play){
                ui->audio_list->topLevelItem(next_play)->setForeground(i,Qt::white);
                ui->audio_list->topLevelItem(next_play)->setBackground(i,QBrush(QColor(255, 100, 103)));
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
    vuMeterL->setLevel(currentVU_L);
    vuMeterR->setLevel(currentVU_R);
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

    if(isPlaying){
        ui->groupBox->setStyleSheet("QGroupBox:title {background-color: red;}");
    } else {
        ui->groupBox->setStyleSheet("");
    }

    if(audioplayer1.isStopped() && audioplayer2.isStopped() && isPlaying && !SayingTimer) {
        current_play += 1;
        if(current_play>(playlist.size()-1)) current_play = 0;
        next();
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

void MainWindow::showConfigDialog()
{
    ConfigDialog configDialog;
    configDialog.setWindowTitle(tr("Configurar"));

    QRect parentRect = this->geometry();
    int x = parentRect.center().x() - configDialog.width() / 2;
    int y = parentRect.center().y() - configDialog.height() / 2;
    configDialog.move(x, y);

    configDialog.exec();
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
