#include "OthelloBoardWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

OthelloBoardWidget::OthelloBoardWidget(QWidget *parent)
    : QWidget(parent)
{
    // init empty 8x8
    m_board = QVector<QVector<int>>(8, QVector<int>(8, 0));
    setMouseTracking(true);
}

void OthelloBoardWidget::setBoard(const QVector<QVector<int>> &board)
{
    if (board.size() != 8) return;
    for (const auto &row : board)
        if (row.size() != 8) return;

    m_board = board;
    update();
}

void OthelloBoardWidget::setLegalMoves(const QVector<QPoint> &moves)
{
    // Expect QPoint(c,r)
    m_legal = moves;
    update();
}

void OthelloBoardWidget::setMyTurn(bool myTurn)
{
    m_myTurn = myTurn;
    update();
}

void OthelloBoardWidget::setWaiting(bool waiting)
{
    m_waiting = waiting;
    update();
}

QRect OthelloBoardWidget::boardRect() const
{
    // square board centered
    int side = qMin(width(), height());
    int margin = qMax(12, side / 30);
    side -= 2 * margin;

    int x = (width() - side) / 2;
    int y = (height() - side) / 2;
    return QRect(x, y, side, side);
}

QRect OthelloBoardWidget::cellRect(int r, int c) const
{
    QRect br = boardRect();
    int cell = br.width() / 8;
    return QRect(br.x() + c * cell, br.y() + r * cell, cell, cell);
}

bool OthelloBoardWidget::isLegalMove(int r, int c) const
{
    return m_legal.contains(QPoint(c, r));
}

bool OthelloBoardWidget::pointToCell(const QPoint &p, int &r, int &c) const
{
    QRect br = boardRect();
    if (!br.contains(p)) return false;

    int cell = br.width() / 8;
    c = (p.x() - br.x()) / cell;
    r = (p.y() - br.y()) / cell;

    if (r < 0 || r > 7 || c < 0 || c > 7) return false;
    return true;
}

void OthelloBoardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    int r=-1, c=-1;
    if (!pointToCell(event->pos(), r, c)) return;

    // Only allow clicking legal moves when it is your turn and not waiting
    if (!m_waiting && m_myTurn && isLegalMove(r, c)) {
        emit cellClicked(r, c);
    }
}

void OthelloBoardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRect br = boardRect();

    // background
    p.fillRect(rect(), palette().window());

    // board background (green-ish without hardcoding specific color names too much)
    // We'll use a neutral brush derived from palette to avoid "hard-coded colors" requirement isn't here (only python plots).
    QBrush boardBrush(QColor(40, 120, 70));
    p.setBrush(boardBrush);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(br, 10, 10);

    // grid
    p.setPen(QPen(QColor(0,0,0,60), 1));
    int cell = br.width() / 8;
    for (int i = 0; i <= 8; ++i) {
        int x = br.x() + i * cell;
        int y = br.y() + i * cell;
        p.drawLine(x, br.y(), x, br.bottom());
        p.drawLine(br.x(), y, br.right(), y);
    }

    // Draw legal move hints (small dots)
    for (const QPoint &mv : m_legal) {
        int c = mv.x();
        int r = mv.y();
        QRect cr = cellRect(r, c);
        int d = qMax(6, cr.width() / 7);
        QPoint center = cr.center();

        QColor hintColor = m_myTurn && !m_waiting ? QColor(255, 220, 120, 200)
                                                  : QColor(255, 255, 255, 120);
        p.setBrush(hintColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, d/2, d/2);
    }

    // pieces
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int v = m_board[r][c];
            if (v == 0) continue;

            QRect cr = cellRect(r, c);
            int pad = qMax(6, cr.width() / 10);
            QRect disc = cr.adjusted(pad, pad, -pad, -pad);

            if (v == 1) {
                p.setBrush(QColor(20,20,20));
                p.setPen(QPen(QColor(0,0,0,120), 2));
            } else { // v == 2
                p.setBrush(QColor(245,245,245));
                p.setPen(QPen(QColor(0,0,0,80), 2));
            }
            p.drawEllipse(disc);
        }
    }

    // overlay text when waiting / not your turn
    if (m_waiting) {
        p.setPen(QColor(0,0,0,170));
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(f.pointSize() + 4);
        p.setFont(f);
        p.drawText(br, Qt::AlignCenter, "Waiting for opponent...");
    } else if (!m_myTurn) {
        p.setPen(QColor(0,0,0,130));
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(br.adjusted(0, 0, 0, -br.height()/2 + 24),
                   Qt::AlignHCenter | Qt::AlignBottom,
                   "Opponent's turn");
    }
}
