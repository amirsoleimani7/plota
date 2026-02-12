#ifndef OTHELLOLOGIC_H
#define OTHELLOLOGIC_H

#include <QJsonObject>
#include <QVector>
#include <QPair>

class OthelloGame
{
public:
    enum Cell : int {
        Empty = 0,
        Black = 1,
        White = 2
    };

    OthelloGame();

    // Reset to initial 4 discs, black starts
    void reset();

    // Whose turn is it now?
    Cell currentPlayer() const { return m_current; }

    // Board access
    Cell at(int r, int c) const;
    void setAt(int r, int c, Cell v);

    // Returns list of legal moves for a player: (row, col)
    QVector<QPair<int,int>> legalMoves(Cell player) const;

    // Convenience: legal moves for current player
    QVector<QPair<int,int>> legalMoves() const { return legalMoves(m_current); }

    // Is a move legal?
    bool isLegalMove(Cell player, int r, int c) const;

    // Apply move for current player.
    // Returns true if move applied, false if illegal.
    bool applyMove(int r, int c);

    // Pass turn if current player has no legal moves.
    // Returns true if pass happened, false if player still had moves.
    bool passIfNoMoves();

    // Game is over if neither player has legal moves.
    bool isGameOver() const;

    // Count discs
    int count(Cell who) const;

    // Who is winner? returns Empty for draw, or Black/White
    Cell winner() const;

    // JSON helpers for server messages
    // board is 8x8 ints: 0 empty, 1 black, 2 white
    QJsonObject toJsonState(bool includeLegalMovesForCurrent = true) const;

    // Utility
    static bool inBounds(int r, int c);

private:
    QVector<Cell> m_board; // size 64
    Cell m_current = Black;

    // returns list of directions (dr,dc) where flips would occur
    QVector<QPair<int,int>> flipDirections(Cell player, int r, int c) const;

    // flips discs along one direction, assumes direction is valid
    int flipLine(Cell player, int r, int c, int dr, int dc);

    static Cell opponent(Cell p);
};

#endif // OTHELLOLOGIC_H
