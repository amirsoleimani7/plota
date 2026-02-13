#include "StackAnimator.h"

#include <QStackedWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

StackAnimator::StackAnimator(QObject *parent) : QObject(parent) {}

void StackAnimator::go(QStackedWidget *stack, QWidget *target, Direction dir, int durationMs)
{
    if (!stack || !target) return;
    if (m_animating) { stack->setCurrentWidget(target); return; }
    if (stack->currentWidget() == target) return;

    QWidget *from = stack->currentWidget();
    QWidget *to = target;

    m_animating = true;

    // Ensure target is visible for animation
    stack->setCurrentWidget(to);

    // Opacity effects
    auto *fromEff = new QGraphicsOpacityEffect(from);
    auto *toEff = new QGraphicsOpacityEffect(to);
    from->setGraphicsEffect(fromEff);
    to->setGraphicsEffect(toEff);
    fromEff->setOpacity(1.0);
    toEff->setOpacity(0.0);

    // Slide (small distance) without fighting layouts too much
    const QPoint basePos = to->pos();
    QPoint offset(0, 0);
    const int d = 18;
    switch (dir) {
    case Left:  offset = QPoint(d, 0); break;
    case Right: offset = QPoint(-d, 0); break;
    case Up:    offset = QPoint(0, d); break;
    case Down:  offset = QPoint(0, -d); break;
    default:    break;
    }
    to->move(basePos + offset);

    auto *fadeOut = new QPropertyAnimation(fromEff, "opacity");
    fadeOut->setDuration(durationMs);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);

    auto *fadeIn = new QPropertyAnimation(toEff, "opacity");
    fadeIn->setDuration(durationMs);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    auto *slideIn = new QPropertyAnimation(to, "pos");
    slideIn->setDuration(durationMs);
    slideIn->setStartValue(basePos + offset);
    slideIn->setEndValue(basePos);
    slideIn->setEasingCurve(QEasingCurve::OutCubic);

    auto *group = new QParallelAnimationGroup(stack);
    group->addAnimation(fadeOut);
    group->addAnimation(fadeIn);
    if (dir != NoSlide) group->addAnimation(slideIn);

    QObject::connect(group, &QParallelAnimationGroup::finished, stack, [=]() {
        // Restore effects
        from->setGraphicsEffect(nullptr);
        to->setGraphicsEffect(nullptr);
        fromEff->deleteLater();
        toEff->deleteLater();

        // Finalize current widget (already set, but keep it explicit)
        stack->setCurrentWidget(to);

        m_animating = false;
        group->deleteLater();
    });

    group->start();
}
