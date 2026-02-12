#include "OthelloLogic.h"

#include <QJsonArray>

static constexpr int N = 8;

OthelloGame::OthelloGame()
{
    reset();
}

void OthelloGame::reset()
{
    m_board = QVector<Cell>(N * N, Empty);

    // Standard initial position (0-indexed):
    // (3,3)=White, (3,4)=Black, (4,3)=Black, (4,4)=White
    setAt(3,3, White);
    setAt(3,4, Black);
    setAt(4,3, Black);
    setAt(4,4, White);

    m_current = Black;
}

bool OthelloGame::inBounds(int r, int c)
{
    return (r >= 0 && r < N && c >= 0 && c < N);
}

OthelloGame::Cell OthelloGame::at(int r, int c) const
{
    if (!inBounds(r,c)) return Empty;
    return m_board[r * N + c];
}

void OthelloGame::setAt(int r, int c, Cell v)
{
    if (!inBounds(r,c)) return;
    m_board[r * N + c] = v;
}

OthelloGame::Cell OthelloGame::opponent(Cell p)
{
    return (p == Black) ? White : Black;
}

QVector<QPair<int,int>> OthelloGame::flipDirections(Cell player, int r, int c) const
{
    QVector<QPair<int,int>> dirs;
    if (!inBounds(r,c)) return dirs;
    if (at(r,c) != Empty) return dirs;

    const Cell opp = opponent(player);

    // 8 directions
    static const int DR[8] = {-1,-1,-1, 0,0, 1,1,1};
    static const int DC[8] = {-1, 0, 1,-1,1,-1,0,1};

    for (int k=0;k<8;k++) {
        int dr = DR[k], dc = DC[k];

        int rr = r + dr;
        int cc = c + dc;

        // first step must be opponent
        if (!inBounds(rr,cc) || at(rr,cc) != opp) continue;

        // keep going until we hit player's disc or empty/out
        rr += dr; cc += dc;
        while (inBounds(rr,cc)) {
            Cell v = at(rr,cc);
            if (v == Empty) break;
            if (v == player) {
                dirs.append({dr,dc});
                break;
            }
            rr += dr; cc += dc;
        }
    }

    return dirs;
}

bool OthelloGame::isLegalMove(Cell player, int r, int c) const
{
    return !flipDirections(player, r, c).isEmpty();
}

QVector<QPair<int,int>> OthelloGame::legalMoves(Cell player) const
{
    QVector<QPair<int,int>> moves;
    for (int r=0;r<N;r++) {
        for (int c=0;c<N;c++) {
            if (isLegalMove(player, r, c))
                moves.append({r,c});
        }
    }
    return moves;
}

int OthelloGame::flipLine(Cell player, int r, int c, int dr, int dc)
{
    const Cell opp = opponent(player);

    int rr = r + dr;
    int cc = c + dc;

    // we assume direction is valid so first cells are opponent and ends with player
    int flipped = 0;
    while (inBounds(rr,cc) && at(rr,cc) == opp) {
        setAt(rr,cc, player);
        flipped++;
        rr += dr; cc += dc;
    }
    return flipped;
}

bool OthelloGame::applyMove(int r, int c)
{
    auto dirs = flipDirections(m_current, r, c);
    if (dirs.isEmpty()) return false;

    // place disc
    setAt(r,c, m_current);

    // flip along each direction
    for (auto d : dirs) {
        flipLine(m_current, r, c, d.first, d.second);
    }

    // switch turn
    m_current = opponent(m_current);

    // auto-pass if next player has no moves but game not over
    passIfNoMoves();

    return true;
}

bool OthelloGame::passIfNoMoves()
{
    if (!legalMoves(m_current).isEmpty())
        return false;

    // current player has no moves -> pass
    m_current = opponent(m_current);
    return true;
}

bool OthelloGame::isGameOver() const
{
    return legalMoves(Black).isEmpty() && legalMoves(White).isEmpty();
}

int OthelloGame::count(Cell who) const
{
    int cnt = 0;
    for (Cell v : m_board) {
        if (v == who) cnt++;
    }
    return cnt;
}

OthelloGame::Cell OthelloGame::winner() const
{
    int b = count(Black);
    int w = count(White);
    if (b == w) return Empty;
    return (b > w) ? Black : White;
}

QJsonObject OthelloGame::toJsonState(bool includeLegalMovesForCurrent) const
{
    QJsonObject st;

    // board
    QJsonArray rows;
    for (int r=0;r<N;r++) {
        QJsonArray row;
        for (int c=0;c<N;c++) {
            row.append((int)at(r,c));
        }
        rows.append(row);
    }
    st["board"] = rows;

    // turn
    st["turn"] = (int)m_current;

    // score
    st["black"] = count(Black);
    st["white"] = count(White);

    // game over / winner
    bool over = isGameOver();
    st["gameOver"] = over;
    if (over) {
        st["winner"] = (int)winner(); // 0 draw, 1 black, 2 white
    }

    // legal moves for current player (optional)
    if (includeLegalMovesForCurrent) {
        QJsonArray m;
        auto moves = legalMoves(m_current);
        for (auto rc : moves) {
            QJsonObject mv;
            mv["r"] = rc.first;
            mv["c"] = rc.second;
            m.append(mv);
        }
        st["legalMoves"] = m;
    }

    return st;
}
