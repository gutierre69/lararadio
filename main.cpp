#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QTranslator>
#include <QSplashScreen>
#include <QTimer>
#include <QObject>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORMTHEME", QByteArray("gtk3"));
    qputenv("GTK_THEME", QByteArray("Adwaita:dark"));
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("org.kde.desktop"));
    qputenv("KDE_COLOR_SCHEME", QByteArray("Dark"));

    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("LaraRadio");
    QCoreApplication::setApplicationName("LaraRadio");

    // QTranslator translator;
    // translator.load(":/languages/en_US.qm");
    // a.installTranslator(&translator);

    MainWindow w;

    QPixmap pixmap(":/images/splash-02.png");
    QPixmap pixmapForSplash = pixmap.scaled(500, 300);
    QSplashScreen splash(pixmapForSplash);


    QStringList styles = QStyleFactory::keys();

    if (styles.contains("gtk3", Qt::CaseInsensitive)) {
        a.setStyle(QStyleFactory::create("gtk3"));
    } else {
        a.setStyle(QStyleFactory::create("Fusion"));

        QPalette darkPalette;

        // Cores baseadas no tema Adwaita Dark
        QColor backgroundColor(48, 48, 48);
        QColor baseColor(36, 36, 36);
        QColor textColor(220, 220, 220);
        QColor highlightColor(85, 170, 255);
        QColor disabledTextColor(127, 127, 127);
        QColor buttonColor(64, 64, 64);

        darkPalette.setColor(QPalette::Window, backgroundColor);
        darkPalette.setColor(QPalette::WindowText, textColor);
        darkPalette.setColor(QPalette::Base, baseColor);
        darkPalette.setColor(QPalette::AlternateBase, backgroundColor);
        darkPalette.setColor(QPalette::ToolTipBase, textColor);
        darkPalette.setColor(QPalette::ToolTipText, textColor);
        darkPalette.setColor(QPalette::Text, textColor);
        darkPalette.setColor(QPalette::Button, buttonColor);
        darkPalette.setColor(QPalette::ButtonText, textColor);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, highlightColor);
        darkPalette.setColor(QPalette::Highlight, highlightColor);
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);

        // Desabilitados
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledTextColor);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledTextColor);

        a.setPalette(darkPalette);
        a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    }

    splash.show();
    a.processEvents();

    // Simulate some work
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        splash.finish(&w);
        splash.close();
        w.show();
    });
    timer.start(2000);

    return a.exec();
}
