#include "StackAnimator.h"

#include <QStackedWidget>
#include <QLabel>
#include <QPixmap>
#include <QPointer>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QEasingCurve>

StackAnimator::StackAnimator(QObject *parent) : QObject(parent) {}

void StackAnimator::go(QStackedWidget *stack, QWidget *target, Direction dir, int durationMs)
{
    if (!stack || !target) return;

    QWidget *current = stack->currentWidget();
    if (current == target) return;

    // If running, stop safely
    if (m_group) {
        m_group->stop();
        m_group->deleteLater();
        m_group = nullptr;
    }

    QPointer<QWidget> from = current;
    QPointer<QWidget> to   = target;
    QPointer<QWidget> viewport = stack; // parent for overlays

    if (!from || !to || !viewport) {
        stack->setCurrentWidget(target);
        return;
    }

    // Render pixmaps (snapshots)
    QPixmap fromPix(from->size());
    fromPix.fill(Qt::transparent);
    from->render(&fromPix);

    // Switch to target so it can render (but we'll cover with overlays)
    stack->setCurrentWidget(to);
    QPixmap toPix(to->size());
    toPix.fill(Qt::transparent);
    to->render(&toPix);

    // Create overlay labels over the stacked widget
    auto *fromShot = new QLabel(stack);
    auto *toShot   = new QLabel(stack);
    fromShot->setPixmap(fromPix);
    toShot->setPixmap(toPix);

    fromShot->setScaledContents(false);
    toShot->setScaledContents(false);

    fromShot->setGeometry(stack->rect());
    toShot->setGeometry(stack->rect());

    fromShot->show();
    toShot->show();
    fromShot->raise();
    toShot->raise();

    // Prepare slide positions
    const QRect base = stack->rect();
    QRect fromEnd = base;
    QRect toStart = base;

    int dx = 0, dy = 0;
    const int w = base.width();
    const int h = base.height();

    switch (dir) {
    case Left:   dx = -w; dy = 0;  break;  // new page comes from right, old goes left
    case Right:  dx =  w; dy = 0;  break;
    case Up:     dx = 0;  dy = -h; break;
    case Down:   dx = 0;  dy =  h; break;
    case NoSlide:
    default:     dx = 0;  dy = 0;  break;
    }

    // For "Left": old moves left (-w), new starts right (+w) and moves to base.
    fromEnd = base.translated(dx, dy);
    toStart = base.translated(-dx, -dy);

    fromShot->setGeometry(base);
    toShot->setGeometry(toStart);

    // Animate geometries (safe: overlays only)
    auto *fromMove = new QPropertyAnimation(fromShot, "geometry");
    fromMove->setDuration(durationMs);
    fromMove->setStartValue(base);
    fromMove->setEndValue(fromEnd);
    fromMove->setEasingCurve(QEasingCurve::OutCubic);

    auto *toMove = new QPropertyAnimation(toShot, "geometry");
    toMove->setDuration(durationMs);
    toMove->setStartValue(toStart);
    toMove->setEndValue(base);
    toMove->setEasingCurve(QEasingCurve::OutCubic);

    // Fade in the new overlay slightly (optional but nice)
    // We'll use windowOpacity property of QLabel (works on QWidget)
    toShot->setWindowOpacity(0.0);

    auto *toFade = new QPropertyAnimation(toShot, "windowOpacity");
    toFade->setDuration(durationMs);
    toFade->setStartValue(0.0);
    toFade->setEndValue(1.0);
    toFade->setEasingCurve(QEasingCurve::OutCubic);

    m_group = new QParallelAnimationGroup(stack);
    if (dir != NoSlide) {
        m_group->addAnimation(fromMove);
        m_group->addAnimation(toMove);
    }
    m_group->addAnimation(toFade);

    QObject::connect(m_group, &QParallelAnimationGroup::finished, stack, [this, fromShot, toShot]() {
        // Clean overlays
        fromShot->deleteLater();
        toShot->deleteLater();

        if (m_group) {
            m_group->deleteLater();
            m_group = nullptr;
        }
    });

    m_group->start();
}
