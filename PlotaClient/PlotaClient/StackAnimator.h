#pragma once
#include <QObject>

class QStackedWidget;
class QParallelAnimationGroup;

class StackAnimator : public QObject
{
    Q_OBJECT
public:
    enum Direction { NoSlide, Left, Right, Up, Down };

    explicit StackAnimator(QObject *parent = nullptr);

    void go(QStackedWidget *stack,
            QWidget *target,
            Direction dir = Left,
            int durationMs = 220);

private:
    QParallelAnimationGroup *m_group = nullptr;
};
