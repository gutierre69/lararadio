#ifndef TIMERCLOCK_H
#define TIMERCLOCK_H

class TimerClock: public QObject
{
    Q_OBJECT
public:
    TimerClock();

public slots:
    void getTime();
    void getSeparateTime();

signals:
    void updateTime(QString newValue);
    void updateSeparateTime(QTime time);

private:
    int m_value;
};

#endif // TIMERCLOCK_H
