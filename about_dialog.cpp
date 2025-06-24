#include "about_dialog.h"
#include "ui_about_dialog.h"
#include <QPixmap>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setFixedSize(size());

    QPixmap pixmapLogo = QPixmap(":/images/complete_logo.png");
    ui->logo->setScaledContents(true);
    ui->logo->setPixmap( pixmapLogo.scaled(pixmapLogo.width(), pixmapLogo.height(), Qt::KeepAspectRatio) );

    ui->creator->setText(tr("por:")+" <a href=\"https://github.com/gutierre69\">Carlos Gutierre</a>");
    // ui->creator->setAlignment(Qt::AlignCenter);
    ui->creator->setOpenExternalLinks(true);

    ui->version->setAlignment(Qt::AlignCenter);
    ui->version->setText( tr("Versão: ") + QString(APP_FULL_VERSION) );

    ui->website->setText("<a href=\"https://lararadio.com\">https://lararadio.com</a>");
    // ui->website->setAlignment(Qt::AlignCenter);
    ui->website->setOpenExternalLinks(true);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
