#pragma once

#include <QFrame>

class QLabel;

class GameCardWidget : public QFrame
{
    Q_OBJECT
public:
    explicit GameCardWidget(QWidget *parent = nullptr);

    void setImage(const QString &resourcePath);
    void setTitle(const QString &t);
    void setSubtitle(const QString &t);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void updateHover(bool on);
    void updateImagePixmap();

private:
    QLabel *m_img = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_sub = nullptr;

    QString m_imgPath;
};
