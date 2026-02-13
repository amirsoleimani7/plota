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

    if (btn->property(kFxInstalledProp).toBool()) return;
    btn->setProperty(kFxInstalledProp, true);

    // Put everything on a single graphics effect stack:
    // QWidget supports only ONE graphicsEffect, so we use DropShadow and animate its offset + blur
    auto *shadow = new QGraphicsDropShadowEffect(btn);
    shadow->setBlurRadius(5);
    shadow->setOffset(0, 2);      // "resting" shadow
    // shadow->setColor(...)       // optional; let default/QSS handle look
    btn->setGraphicsEffect(shadow);

    // Opacity (we'll animate via a 2nd effect? can't, only one effect)
    // So instead: animate "strength" by blur+offset and use QSS :pressed background.
    // If you still want opacity fade, do it in QSS with QPushButton:pressed { ... }.

    // Animations for "press down" feel (layout-safe)
    auto *pressOffset = new QPropertyAnimation(shadow, "offset", btn);
    pressOffset->setDuration(durationMs);
    pressOffset->setStartValue(QPointF(0, 4));
    pressOffset->setEndValue(QPointF(0, 1)); // shadow closer => looks pressed
    pressOffset->setEasingCurve(QEasingCurve::OutCubic);

    auto *pressBlur = new QPropertyAnimation(shadow, "blurRadius", btn);
    pressBlur->setDuration(durationMs);
    pressBlur->setStartValue(18.0);
    pressBlur->setEndValue(8.0);
    pressBlur->setEasingCurve(QEasingCurve::OutCubic);

    auto *releaseOffset = new QPropertyAnimation(shadow, "offset", btn);
    releaseOffset->setDuration(durationMs);
    releaseOffset->setStartValue(QPointF(0, 1));
    releaseOffset->setEndValue(QPointF(0, 4));
    releaseOffset->setEasingCurve(QEasingCurve::OutCubic);

    auto *releaseBlur = new QPropertyAnimation(shadow, "blurRadius", btn);
    releaseBlur->setDuration(durationMs);
    releaseBlur->setStartValue(8.0);
    releaseBlur->setEndValue(18.0);
    releaseBlur->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(btn, &QPushButton::pressed, btn, [=]() {
        releaseOffset->stop();
        releaseBlur->stop();
        pressOffset->stop();
        pressBlur->stop();
        pressOffset->start();
        pressBlur->start();
    });

    QObject::connect(btn, &QPushButton::released, btn, [=]() {
        pressOffset->stop();
        pressBlur->stop();
        releaseOffset->stop();
        releaseBlur->stop();
        releaseOffset->start();
        releaseBlur->start();
    });
}
