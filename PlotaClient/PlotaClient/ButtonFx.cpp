#include "ButtonFx.h"

#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

void ButtonFx::install(QPushButton *btn, int durationMs)
{
    if (!btn) return;

    auto *eff = qobject_cast<QGraphicsOpacityEffect*>(btn->graphicsEffect());
    if (!eff) {
        eff = new QGraphicsOpacityEffect(btn);
        eff->setOpacity(1.0);
        btn->setGraphicsEffect(eff);
    }

    auto *fadeDown = new QPropertyAnimation(eff, "opacity", btn);
    fadeDown->setDuration(durationMs);
    fadeDown->setStartValue(1.0);
    fadeDown->setEndValue(0.86);
    fadeDown->setEasingCurve(QEasingCurve::OutCubic);

    auto *fadeUp = new QPropertyAnimation(eff, "opacity", btn);
    fadeUp->setDuration(durationMs);
    fadeUp->setStartValue(0.86);
    fadeUp->setEndValue(1.0);
    fadeUp->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(btn, &QPushButton::pressed, btn, [fadeDown, fadeUp]() {
        fadeUp->stop();
        fadeDown->start();
    });
    QObject::connect(btn, &QPushButton::released, btn, [fadeDown, fadeUp]() {
        fadeDown->stop();
        fadeUp->start();
    });
}
