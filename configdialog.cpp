#include "configdialog.h"
#include "ui_configdialog.h"

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    setFixedSize(size());

    QCoreApplication::setOrganizationName("LaraRadio");
    QCoreApplication::setApplicationName("LaraRadio");
    QSettings settings;

    ui->fadeInOut->setValue( settings.value("volume/TransitionAudioTime").toInt() );
    ui->speedFade->setValue( settings.value("volume/speedFade").toInt() );

    ui->music_path->setText( settings.value("files/defaultDir").toString() );
    ui->jingle_path->setText( settings.value("files/jingleDir").toString() );
    ui->time_path->setText( settings.value("files/audioTimeDir").toString() );

    ui->onair_script->setText( settings.value("onair/script").toString() );
    ui->onair_param_on->setText( settings.value("onair/param_on", "on1").toString() );
    ui->onair_param_off->setText( settings.value("onair/param_off", "off1").toString() );
    // Exemplo dinâmico (sem nome de usuário hardcoded — usa ~/ que resolve pro home de quem roda)
    ui->label_9->setText( tr("Script ON AIR (ex: ~/bin/usbrelay2)") );

    ui->sayClock->setChecked( settings.value("volume/sayClock").toBool() );
    ui->sayClockFade->setChecked( settings.value("volume/sayClockFade").toBool() );

    ui->stopFade->setChecked( settings.value("volume/stopFade").toBool() );
    ui->talkFade->setChecked( settings.value("volume/talkFade").toBool() );

    ui->autosaveEnabled->setChecked( settings.value("autosave/enabled").toBool() );

    ui->autoJingleEnabled->setChecked( settings.value("autojingle/enabled").toBool() );
    ui->autoJingleCount->setValue( settings.value("autojingle/count", 1).toInt() );
    ui->autoJingleInterval->setValue( settings.value("autojingle/interval", 5).toInt() );

    ui->streamUrl->setText( settings.value("stream/url").toString() );
    ui->streamUser->setText( settings.value("stream/user").toString() );
    ui->streamPass->setText( settings.value("stream/pass").toString() );
    ui->streamName->setText( settings.value("stream/name").toString() );
    ui->streamDesc->setText( settings.value("stream/description").toString() );

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::accept()
{
    settings.setValue("volume/TransitionAudioTime", ui->fadeInOut->value());
    settings.setValue("volume/speedFade", ui->speedFade->value());
    settings.setValue("files/defaultDir", ui->music_path->text());
    settings.setValue("files/jingleDir", ui->jingle_path->text());
    settings.setValue("files/audioTimeDir", ui->time_path->text());
    settings.setValue("onair/script", ui->onair_script->text());
    settings.setValue("onair/param_on", ui->onair_param_on->text());
    settings.setValue("onair/param_off", ui->onair_param_off->text());
    settings.setValue("volume/sayClock", ui->sayClock->isChecked());
    settings.setValue("volume/sayClockFade", ui->sayClockFade->isChecked());
    settings.setValue("volume/stopFade", ui->stopFade->isChecked());
    settings.setValue("volume/talkFade", ui->talkFade->isChecked());
    settings.setValue("autosave/enabled", ui->autosaveEnabled->isChecked());
    settings.setValue("autojingle/enabled", ui->autoJingleEnabled->isChecked());
    settings.setValue("autojingle/count", ui->autoJingleCount->value());
    settings.setValue("autojingle/interval", ui->autoJingleInterval->value());
    settings.setValue("stream/url", ui->streamUrl->text().trimmed());
    settings.setValue("stream/user", ui->streamUser->text().trimmed());
    settings.setValue("stream/pass", ui->streamPass->text());
    settings.setValue("stream/name", ui->streamName->text().trimmed());
    settings.setValue("stream/description", ui->streamDesc->text().trimmed());

    // Grava no disco JÁ — o MainWindow reavalia o botão de stream logo após
    // o exec() e precisa ver os valores novos (sem sync, só o destrutor grava).
    settings.sync();

    this->close();
}

void ConfigDialog::on_btn_searchMusicPath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Selecionar pasta de músicas"), QDir::homePath());
    if (!dir.isEmpty()) {
        ui->music_path->setText(dir);
    }
}

void ConfigDialog::on_btn_searchJinglePath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Selecionar pasta de vinhetas"), QDir::homePath());
    if (!dir.isEmpty()) {
        ui->jingle_path->setText(dir);
    }
}

void ConfigDialog::on_btn_export_log_clicked()
{
    emit exportLogRequested();
}

void ConfigDialog::on_btn_searchTimePath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Selecionar pasta de Locução de Hora"), settings.value("files/audioTimeDir", QDir::homePath()).toString());
    if (!dir.isEmpty()) {
        ui->time_path->setText(dir);
    }
}
