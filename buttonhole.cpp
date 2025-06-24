#include "buttonhole.h"

#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QFileDialog>

ButtonHole::ButtonHole(QWidget *parent) : QWidget(parent)
{
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;

    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(1.0f);

    settings = new QSettings("LaraRadio", "LaraRadio", this);

    button = new QPushButton("", this);
    button->resize( width, height );

    button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(button, &QPushButton::customContextMenuRequested, this, [=](const QPoint& pos) { bntContextMenu(pos); });
    connect(button, &QPushButton::clicked, this, [=]() { buttonHoleClick(); });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ButtonHole::flash);
    timer->start(300);
}

void ButtonHole::setPositon(int newX, int newY)
{
    //button->setGeometry(newX, newY, 60, 60);
}

void ButtonHole::setBtnText(QString newText)
{
    text = newText;
    button->setText( text );
}

void ButtonHole::bntContextMenu(QPoint pos)
{
    QMenu *menu = new QMenu(this);

    QAction* loadAudio = new QAction(QIcon(":/images/icons/go-bottom.svg"), "Carregar Áudio", this);
    QAction* playAudio = new QAction(QIcon(":/images/icons/media-playback-start.svg"), "Tocar Áudio", this);
    QAction* stopAudio = new QAction(QIcon(":/images/icons/media-playback-stop.svg"), "Parar Áudio", this);
    QAction* deleteAudio = new QAction(QIcon(":/images/icons/edit-delete.svg"), "Apagar Áudio", this);
    menu->addAction( loadAudio );
    menu->addAction( playAudio );
    menu->addAction( stopAudio );
    menu->addSeparator();
    menu->addAction( deleteAudio );
    menu->popup(button->mapToGlobal(pos));

    connect(loadAudio, &QAction::triggered, this, [=]() {
        QString filename = QFileDialog::getOpenFileName(this, tr("Carregar Audio"), QDir::homePath(), tr("Arquivos de Audio")+" (*.mp3 *.wav *.ogg *.flac *.mp4);;");

        if (!filename.isEmpty()) {
            settings->setValue("buttonhole/btn_"+text, filename);
            button->setStyleSheet("background-color: #fc0; color: #000;");
        }
    });

    connect(deleteAudio, &QAction::triggered, this, [=]() {
        settings->setValue("buttonhole/btn_"+text, "");
        button->setStyleSheet("");
    });

    connect(playAudio, &QAction::triggered, this, [=]() {
        buttonHoleClick();
    });

    connect(stopAudio, &QAction::triggered, this, [=]() {
        buttonHoleStop();
    });
}

void ButtonHole::buttonHoleClick()
{
    // /home/gutierre69/Documentos/Studio/EFEITOS/Brasil_sil_sil.mp3
    filename = settings->value("buttonhole/btn_"+text).toString();
    if(filename=="")
        return;

    player->stop();
    player->setSource( QUrl::fromLocalFile( filename ) );
    player->play();
}

void ButtonHole::buttonHoleStop()
{
    player->stop();
}

void ButtonHole::buttonHoleKeyPress()
{
    if(player->isPlaying()) buttonHoleStop(); else buttonHoleClick();
}

void ButtonHole::flash()
{
    if(player->isPlaying()) {
        if(button->styleSheet()!="") {
            button->setStyleSheet("");
        } else {
            button->setStyleSheet("background-color: #fc0; color: #000;");
        }
        return;
    }

    if(settings->value("buttonhole/btn_"+text).toString()!="") button->setStyleSheet("background-color: #fc0; color: #000;");
}

void ButtonHole::keyPressEvent(QKeyEvent *event){
    if(event->key() == Qt::Key_1) qDebug() << "btn 1";
}
