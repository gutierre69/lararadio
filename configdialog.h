#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDialogButtonBox>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

public slots:
    void accept();
    //void openFileDialog(QMouseEvent event);

private slots:
    void on_btn_searchMusicPath_clicked();
    void on_btn_searchJinglePath_clicked();
    void on_btn_searchTimePath_clicked();

private:
    Ui::ConfigDialog *ui;

    QSettings settings;
};

#endif // CONFIGDIALOG_H
