#pragma once
#include <QObject>
class QPushButton;

class ButtonFx : public QObject {
    Q_OBJECT
public:
    static void install(QPushButton *btn, int durationMs = 200);
};
