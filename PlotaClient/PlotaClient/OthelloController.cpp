#include "OthelloController.h"
#include "ClientProtocol.h"
#include "OthelloPage.h"
#include "OthelloBoardWidget.h"

#include <QJsonArray>
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

    // Board clicks -> send move
    if (m_page && m_page->boardWidget()) {
        connect(m_page->boardWidget(), &OthelloBoardWidget::cellClicked,
                this, [this](int r, int c) {
                    if (!m_proto) return;
                    QJsonObject payload;
                    payload["r"] = r;
                    payload["c"] = c;
                    m_proto->sendMessage("othello_move", payload);
                });
    }
}

void OthelloController::connectUi()
{
    if (!m_page || !m_proto) return;

    connect(m_page, &OthelloPage::createRoomClicked, this, [this]() {
        m_proto->sendMessage("othello_create_room", QJsonObject{});
    });

    connect(m_page, &OthelloPage::joinRoomClicked, this, [this]() {
        QJsonObject payload;
        payload["room"] = m_page->roomCodeInput();
        m_proto->sendMessage("othello_join_room", payload);
    });

    connect(m_page, &OthelloPage::leaveRoomClicked, this, [this]() {
        m_proto->sendMessage("othello_leave_room", QJsonObject{});
    });

    // ✅ Back button: if in room, leave first (clean), then go menu
    connect(m_page, &OthelloPage::backToMenuClicked, this, [this]() {
        // Optional: auto-leave so server removes you from room.
        if (!m_room.isEmpty()) {
            m_proto->sendMessage("othello_leave_room", QJsonObject{});
        }
        resetLocalRoomState();
        emit backToMenuRequested();
    });

    // ✅ Chat send
    connect(m_page, &OthelloPage::chatSendClicked, this, [this](const QString &text) {
        if (!m_proto) return;
        if (m_room.isEmpty()) {
            m_page->setStatus("Join/Create a room first.");
            return;
        }

        QJsonObject payload;
        payload["text"] = text;
        m_proto->sendMessage("othello_chat_send", payload);
    });
}

void OthelloController::connectProtocol()
{
    if (!m_proto) return;
    connect(m_proto, &ClientProtocol::messageReceived,
            this, &OthelloController::handleMessage);
}

void OthelloController::requestStateAndChat()
{
    if (!m_proto) return;
    m_proto->sendMessage("othello_get_state", QJsonObject{});
    m_proto->sendMessage("othello_chat_get", QJsonObject{});
}

void OthelloController::resetLocalRoomState()
{
    m_room.clear();
    m_myColor = 0;

    if (!m_page) return;

    m_page->setInRoom(false);
    m_page->clearChat();
    m_page->setGameState(QVector<QVector<int>>(8, QVector<int>(8, 0)),
                         {}, false, false);
    m_page->setStatus("");
}

void OthelloController::handleMessage(const QString &type,
                                      const QJsonObject &payload)
{
    qDebug() << "[Othello]" << type << payload;

    if (!m_page) return;

    // ---------- room create ----------
    if (type == "othello_create_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Create failed: " + payload.value("error").toString());
            return;
        }

        m_room = payload.value("room").toString();
        m_myColor = payload.value("youAre").toInt();

        m_page->setRoomCode(m_room);
        m_page->setStatus("Room created. Waiting for opponent...");
        m_page->setInRoom(true);

        requestStateAndChat();
        return;
    }

    // ---------- join ----------
    if (type == "othello_join_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Join failed: " + payload.value("error").toString());
            return;
        }

        m_room = payload.value("room").toString();
        m_myColor = payload.value("youAre").toInt();

        m_page->setStatus("Joined room " + m_room);
        m_page->setInRoom(true);

        requestStateAndChat();
        return;
    }

    // ---------- leave ----------
    if (type == "othello_leave_room_result") {
        if (!payload.value("ok").toBool()) {
            m_page->setStatus("Leave failed: " + payload.value("error").toString());
            return;
        }

        resetLocalRoomState();
        m_page->setStatus("Left room.");
        return;
    }

    // ---------- state broadcast ----------
    if (type == "othello_state") {
        applyState(payload);
        return;
    }

    // ---------- state result ----------
    if (type == "othello_state_result") {
        if (!payload.value("ok").toBool(false)) {
            m_page->setStatus("State load failed: " + payload.value("error").toString());
            return;
        }
        QJsonObject state = payload.value("state").toObject();
        applyState(state);
        return;
    }

    // ✅ ---------- chat broadcast ----------
    if (type == "othello_chat") {
        // { room, from, text, ts }
        QString room = payload.value("room").toString();
        if (!m_room.isEmpty() && room != m_room) return;

        QString from = payload.value("from").toString();
        QString text = payload.value("text").toString();
        QString ts   = payload.value("ts").toString();

        if (!from.isEmpty() && !text.isEmpty())
            m_page->appendChatMessage(from, text, ts);

        return;
    }

    // ✅ ---------- chat history result ----------
    if (type == "othello_chat_get_result") {
        if (!payload.value("ok").toBool(false)) {
            // not fatal
            return;
        }

        QString room = payload.value("room").toString();
        if (!m_room.isEmpty() && !room.isEmpty() && room != m_room) return;

        QJsonArray arr = payload.value("messages").toArray();
        m_page->clearChat();

        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            QString from = o.value("from").toString();
            QString text = o.value("text").toString();
            QString ts   = o.value("ts").toString();
            if (!from.isEmpty() && !text.isEmpty())
                m_page->appendChatMessage(from, text, ts);
        }

        return;
    }

    // ignore other messages
}

void OthelloController::applyState(const QJsonObject &state)
{
    QString roomInState = state.value("room").toString();

    // if we already have a room, ignore other rooms
    if (!m_room.isEmpty() && roomInState != m_room)
        return;

    // if we don't have room yet, don't adopt state (avoid race)
    if (m_room.isEmpty() && !roomInState.isEmpty())
        return;

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

    // Parse legal moves (server currently sends full legalMoves for current player)
    QVector<QPoint> legal;
    QJsonArray moves = state.value("legalMoves").toArray();
    for (const QJsonValue &mv : moves) {
        QJsonObject o = mv.toObject();
        int r = o.value("r").toInt(-1);
        int c = o.value("c").toInt(-1);
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
            legal.push_back(QPoint(c, r));
    }

    // ✅ Important: only show legal moves on YOUR turn
    QVector<QPoint> shownLegal = (myTurn ? legal : QVector<QPoint>{});
    m_page->setGameState(board, shownLegal, myTurn, waiting);

    // status
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
