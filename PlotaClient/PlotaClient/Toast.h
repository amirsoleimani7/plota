#pragma once
#include <QWidget>

class QLabel;
class QPropertyAnimation;

class Toast : public QWidget
{
    Q_OBJECT
public:
    enum Kind { Info, Success, Warning, Error };

    static void show(QWidget *parent,
                     const QString &text,
                     Kind kind = Info,
                     int ms = 4000);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    explicit Toast(QWidget *parent = nullptr);

    void setKind(Kind kind);
    void start(int ms);

    QLabel *m_label = nullptr;
    QPropertyAnimation *m_fadeIn = nullptr;
    QPropertyAnimation *m_fadeOut = nullptr;
};
