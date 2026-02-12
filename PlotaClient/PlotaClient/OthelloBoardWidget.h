#ifndef OTHELLOBOARDWIDGET_H
#define OTHELLOBOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPoint>

class OthelloBoardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OthelloBoardWidget(QWidget *parent = nullptr);

    // board is 8x8 with values: 0 empty, 1 black, 2 white
    void setBoard(const QVector<QVector<int>> &board);

    // legal moves list as (r,c)
    void setLegalMoves(const QVector<QPoint> &moves);

    // optional status
    void setMyTurn(bool myTurn);
    void setWaiting(bool waiting);

signals:
    void cellClicked(int r, int c);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize minimumSizeHint() const override { return QSize(360, 360); }

private:
    QRect boardRect() const;
    QRect cellRect(int r, int c) const;
    bool pointToCell(const QPoint &p, int &r, int &c) const;
    bool isLegalMove(int r, int c) const;

    QVector<QVector<int>> m_board; // 8x8
    QVector<QPoint> m_legal;       // QPoint(c,r) or (x,y)?? we’ll store as (c,r)? -> we store as (c,r)?? Let's be consistent:
    // We'll store as QPoint(c, r) for fast contains.
    bool m_myTurn = false;
    bool m_waiting = false;
};

#endif // OTHELLOBOARDWIDGET_H
