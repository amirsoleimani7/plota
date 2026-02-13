#include "ButtonFx.h"

#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

static const char* kFxInstalledProp = "_btnFxInstalled";

void ButtonFx::install(QPushButton *btn, int durationMs)
{
    if (!btn) return;

    // prevent double install
    if (btn->property(kFxInstalledProp).toBool()) return;
    btn->setProperty(kFxInstalledProp, true);

    // Opacity effect (visible)
    auto *op = new QGraphicsOpacityEffect(btn);
    op->setOpacity(1.0);
    btn->setGraphicsEffect(op);

    auto *fadeDown = new QPropertyAnimation(op, "opacity", btn);
    fadeDown->setDuration(durationMs);
    fadeDown->setStartValue(1.0);
    fadeDown->setEndValue(0.1);          // <-- MUCH more visible
    fadeDown->setEasingCurve(QEasingCurve::OutCubic);

    auto *fadeUp = new QPropertyAnimation(op, "opacity", btn);
    fadeUp->setDuration(durationMs);
    fadeUp->setStartValue(0.72);
    fadeUp->setEndValue(1.0);
    fadeUp->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(btn, &QPushButton::pressed, btn, [fadeDown, fadeUp]() {
        fadeUp->stop();
        fadeDown->stop();
        fadeDown->start();
    });
    QObject::connect(btn, &QPushButton::released, btn, [fadeDown, fadeUp]() {
        fadeDown->stop();
        fadeUp->stop();
        fadeUp->start();
    });
}
