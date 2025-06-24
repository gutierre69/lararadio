#include <QtWidgets>
#include "timerclock.h"

TimerClock::TimerClock(){
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(getTime()));
    connect(timer, SIGNAL(timeout()), this, SLOT(getSeparateTime()));
    timer->start(1000);
    getTime();
    getSeparateTime();
}

void TimerClock::getTime()
{
    QTime time = QTime::currentTime();
    QString text = time.toString("hh:mm:ss");
    emit updateTime(text);
}

void TimerClock::getSeparateTime()
{
    QTime time = QTime::currentTime();

    emit updateSeparateTime(time);
}
