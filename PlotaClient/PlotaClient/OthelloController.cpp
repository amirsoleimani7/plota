#include "OthelloController.h"
#include "ClientProtocol.h"
#include "OthelloPage.h"
#include "OthelloBoardWidget.h"
#include <QJsonArray>
#include <QSet>

#include <QDebug>

OthelloController::OthelloController(ClientProtocol *proto,
                                     OthelloPage *page,
                                     QObject *parent)
    : QObject(parent),
    m_proto(proto),
    m_page(page)
{
    connectUi();
    connectProtocol();
    if (m_page && m_page->boardWidget()) {
        connect(m_page->boardWidget(), &OthelloBoardWidget::cellClicked,
                this, [this](int r, int c) {
                    QJsonObject payload;
                    payload["r"] = r;
                    payload["c"] = c;
                    m_proto->sendMessage("othello_move", payload);
                });
    }

}

void OthelloController::connectUi()
{
    connect(m_page, &OthelloPage::createRoomClicked, this, [this]() {
        QJsonObject payload;
        m_proto->sendMessage("othello_create_room", payload);
    });

    connect(m_page, &OthelloPage::joinRoomClicked, this, [this]() {
        QJsonObject payload;
        payload["room"] = m_page->roomCodeInput();
        m_proto->sendMessage("othello_join_room", payload);
    });

    connect(m_page, &OthelloPage::leaveRoomClicked, this, [this]() {
        QJsonObject payload;
        m_proto->sendMessage("othello_leave_room", payload);
    });
}

void OthelloController::connectProtocol()
{
    connect(m_proto, &ClientProtocol::messageReceived,
            this, &OthelloController::handleMessage);
}

void OthelloController::handleMessage(const QString &type,
                                      const QJsonObject &payload)
{
    qDebug() << "[Othello]" << type << payload;

    if (type == "othello_create_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Create failed: " +
                              payload.value("error").toString());
            return;
        }

        m_room = payload.value("room").toString();
        m_myColor = payload.value("youAre").toInt();

        m_page->setRoomCode(m_room);
        m_page->setStatus("Room created. Waiting for opponent...");

        m_page->setInRoom(true);
        // ask for state immediately (optional; server also broadcasts)
        m_proto->sendMessage("othello_get_state", QJsonObject{});

    }

    else if (type == "othello_join_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Join failed: " +
                              payload.value("error").toString());
            return;
        }

        m_room = payload.value("room").toString();
        m_myColor = payload.value("youAre").toInt();

        m_page->setStatus("Joined room " + m_room);

        m_page->setInRoom(true);
        m_proto->sendMessage("othello_get_state", QJsonObject{});

    }

    else if (type == "othello_leave_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Leave failed: " +
                              payload.value("error").toString());
            return;
        }

        m_room.clear();
        m_myColor = 0;
        m_page->setStatus("Left room.");

        m_page->setInRoom(false);
        m_page->setGameState(QVector<QVector<int>>(8, QVector<int>(8, 0)),
                             {}, false, false);

    }

    else if (type == "othello_state") {
        // Broadcast state update
        applyState(payload);
    }

    else if (type == "othello_state_result") {
        if (!payload.value("ok").toBool(false)) {
            m_page->setStatus("State load failed: " + payload.value("error").toString());
            return;
        }
        QJsonObject state = payload.value("state").toObject();
        applyState(state);
    }

}

void OthelloController::applyState(const QJsonObject &state)
{
    // state payload example:
    // { room, board[[..]], legalMoves[{r,c}], turn, black, white, waitingForOpponent, gameOver }

    QString roomInState = state.value("room").toString();

    // If we already have a room, ignore other rooms
    if (!m_room.isEmpty() && roomInState != m_room)
        return;

    // If we don't have a room yet but state has a room, adopt it only if it makes sense
    // (this prevents “state arrives before create_room_result” confusion)
    if (m_room.isEmpty() && !roomInState.isEmpty()) {
        // don’t auto-adopt unless we are already "in room" logically.
        // safest behavior: just show a generic status and skip board update
        // until we have create/join result setting m_room.
        return;
    }

    bool waiting = state.value("waitingForOpponent").toBool(false);
    int turn = state.value("turn").toInt(0);
    int black = state.value("black").toInt(0);
    int white = state.value("white").toInt(0);
    bool gameOver = state.value("gameOver").toBool(false);

    bool myTurn = (!waiting && m_myColor != 0 && turn == m_myColor);

    // Parse board
    QVector<QVector<int>> board(8, QVector<int>(8, 0));
    QJsonArray rows = state.value("board").toArray();
    if (rows.size() == 8) {
        for (int r = 0; r < 8; ++r) {
            QJsonArray cols = rows[r].toArray();
            if (cols.size() == 8) {
                for (int c = 0; c < 8; ++c) {
                    board[r][c] = cols[c].toInt(0);
                }
            }
        }
    }

    // Parse legal moves -> QPoint(c,r)
    QVector<QPoint> legal;
    QJsonArray moves = state.value("legalMoves").toArray();
    for (const QJsonValue &mv : moves) {
        QJsonObject o = mv.toObject();
        int r = o.value("r").toInt(-1);
        int c = o.value("c").toInt(-1);
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
            legal.push_back(QPoint(c, r));
    }

    m_page->setGameState(board, legal, myTurn, waiting);

    // Update status text
    if (gameOver) {
        QString result;
        if (black > white) result = "Black wins!";
        else if (white > black) result = "White wins!";
        else result = "Draw!";
        m_page->setStatus(QString("Game over. Black: %1  White: %2  |  %3")
                              .arg(black).arg(white).arg(result));
        return;
    }

    if (waiting) {
        m_page->setStatus("Waiting for opponent...");
        return;
    }

    QString turnText = myTurn ? "Your turn" : "Opponent's turn";
    m_page->setStatus(QString("Black: %1   White: %2   |   %3")
                          .arg(black)
                          .arg(white)
                          .arg(turnText));
}

