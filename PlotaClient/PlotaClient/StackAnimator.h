#pragma once
#include <QObject>

class QStackedWidget;

class StackAnimator : public QObject
{
    Q_OBJECT
public:
    enum Direction { NoSlide, Left, Right, Up, Down };

    explicit StackAnimator(QObject *parent = nullptr);

    void go(QStackedWidget *stack, QWidget *target,
            Direction dir = NoSlide,
            int durationMs = 220);

private:
    bool m_animating = false;
};
