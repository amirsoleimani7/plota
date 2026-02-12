#include "OthelloRoomService.h"
#include "RoomManager.h"

#include <QTcpSocket>
#include <QJsonDocument>
#include <QDateTime>
#include <QJsonArray>

OthelloRoomService::OthelloRoomService(RoomManager &rooms, QObject *parent)
    : QObject(parent), m_rooms(rooms)
{
    connect(&m_rooms, &RoomManager::roomBecameFull, this, [this](const QString &code){
        startGameIfReady(code);
    });

    connect(&m_rooms, &RoomManager::roomEmptied, this, [this](const QString &code){
        m_games.remove(code);
        m_chat.remove(code);   // ✅ also clear chat history
    });
}

void OthelloRoomService::ensureGameExists(const QString &code)
{
    if (!m_games.contains(code)) {
        OthelloGame g;
        g.reset();
        m_games.insert(code, g);
    }
}

void OthelloRoomService::startGameIfReady(const QString &code)
{
    if (!m_rooms.isFull(code)) return;

    OthelloGame g;
    g.reset();
    m_games.insert(code, g);

    emitState(code);
}

void OthelloRoomService::emitState(const QString &code)
{
    auto it = m_games.find(code);
    if (it == m_games.end()) return;

    QJsonObject st = it.value().toJsonState(true);
    st["room"] = code;
    emit roomStateChanged(code, st);
}

QString OthelloRoomService::createRoom(QTcpSocket *creator)
{
    QString code = m_rooms.createRoom(creator);
    if (code.isEmpty()) return "";

    ensureGameExists(code);
    emit roomCreated(code);

    QJsonObject st = m_games[code].toJsonState(true);
    st["room"] = code;
    st["waitingForOpponent"] = true;
    emit roomStateChanged(code, st);

    return code;
}

bool OthelloRoomService::joinRoom(const QString &code, QTcpSocket *joiner)
{
    if (!m_rooms.joinRoom(code, joiner))
        return false;

    ensureGameExists(code);
    emit roomJoined(code);

    if (m_rooms.isFull(code)) {
        startGameIfReady(code);
    } else {
        emitState(code);
    }

    return true;
}

QString OthelloRoomService::leaveRoom(QTcpSocket *sock)
{
    QString code = m_rooms.leaveRoom(sock);
    if (code.isEmpty()) return "";

    if (m_rooms.getRoom(code)) {
        ensureGameExists(code);
        QJsonObject st = m_games[code].toJsonState(true);
        st["room"] = code;
        st["waitingForOpponent"] = !m_rooms.isFull(code);
        emit roomStateChanged(code, st);
    }

    return code;
}

QString OthelloRoomService::roomOf(QTcpSocket *sock) const
{
    return m_rooms.roomOf(sock);
}

OthelloGame::Cell OthelloRoomService::playerColor(QTcpSocket *sock) const
{
    QString code = roomOf(sock);
    if (code.isEmpty()) return OthelloGame::Empty;

    const RoomManager::Room *r = m_rooms.getRoom(code);
    if (!r) return OthelloGame::Empty;

    if (r->black == sock) return OthelloGame::Black;
    if (r->white == sock) return OthelloGame::White;
    return OthelloGame::Empty;
}

QJsonObject OthelloRoomService::currentStateFor(QTcpSocket *sock, bool includeLegalMoves) const
{
    QJsonObject st;

    QString code = roomOf(sock);
    if (code.isEmpty()) {
        st["ok"] = false;
        st["error"] = "NOT_IN_ROOM";
        return st;
    }

    auto it = m_games.find(code);
    if (it == m_games.end()) {
        st["ok"] = false;
        st["error"] = "NO_GAME";
        return st;
    }

    st["ok"] = true;
    QJsonObject game = it.value().toJsonState(includeLegalMoves);
    game["room"] = code;
    game["youAre"] = (int)playerColor(sock);
    game["waitingForOpponent"] = !m_rooms.isFull(code);
    st["state"] = game;
    return st;
}

QJsonObject OthelloRoomService::tryMove(QTcpSocket *sock, int r, int c)
{
    QJsonObject out;

    QString code = roomOf(sock);
    if (code.isEmpty()) {
        out["ok"] = false;
        out["error"] = "NOT_IN_ROOM";
        return out;
    }

    if (!m_rooms.isFull(code)) {
        out["ok"] = false;
        out["error"] = "WAITING_FOR_OPPONENT";
        return out;
    }

    auto it = m_games.find(code);
    if (it == m_games.end()) {
        out["ok"] = false;
        out["error"] = "NO_GAME";
        return out;
    }

    OthelloGame &g = it.value();
    OthelloGame::Cell me = playerColor(sock);

    if (me == OthelloGame::Empty) {
        out["ok"] = false;
        out["error"] = "NOT_A_PLAYER";
        return out;
    }

    if (g.currentPlayer() != me) {
        out["ok"] = false;
        out["error"] = "NOT_YOUR_TURN";
        return out;
    }

    if (!OthelloGame::inBounds(r,c)) {
        out["ok"] = false;
        out["error"] = "OUT_OF_BOUNDS";
        return out;
    }

    if (!g.applyMove(r,c)) {
        out["ok"] = false;
        out["error"] = "ILLEGAL_MOVE";
        return out;
    }

    emitState(code);

    out["ok"] = true;
    out["state"] = g.toJsonState(true);
    return out;
}

// =====================================================
// ✅ CHAT IMPLEMENTATION (CORRECT FOR YOUR ROOM SYSTEM)
// =====================================================

static void sendMessage(QTcpSocket *sock, const QString &type, const QJsonObject &payload)
{
    if (!sock) return;
    QJsonObject msg;
    msg["type"] = type;
    msg["payload"] = payload;
    QByteArray out = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    out.append('\n');
    sock->write(out);
}

QJsonObject OthelloRoomService::broadcastChat(QTcpSocket *sock, const QString &from, const QString &text)
{
    QJsonObject out;

    QString roomCode = roomOf(sock);
    if (roomCode.isEmpty()) {
        out["ok"] = false;
        out["error"] = "NOT_IN_ROOM";
        return out;
    }

    const RoomManager::Room *r = m_rooms.getRoom(roomCode);
    if (!r) {
        out["ok"] = false;
        out["error"] = "ROOM_NOT_FOUND";
        return out;
    }

    QString ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // store message in memory (per-room)
    ChatMessage cm{from, text, ts};
    auto &vec = m_chat[roomCode];
    vec.push_back(cm);

    const int MAX_CHAT = 50;
    if (vec.size() > MAX_CHAT)
        vec.remove(0, vec.size() - MAX_CHAT);

    // broadcast message
    QJsonObject chatPayload;
    chatPayload["room"] = roomCode;
    chatPayload["from"] = from;
    chatPayload["text"] = text;
    chatPayload["ts"] = ts;

    if (r->black) sendMessage(r->black, "othello_chat", chatPayload);
    if (r->white) sendMessage(r->white, "othello_chat", chatPayload);

    out["ok"] = true;
    out["room"] = roomCode;
    return out;
}

QJsonObject OthelloRoomService::getChat(QTcpSocket *sock) const
{
    QJsonObject out;

    QString roomCode = roomOf(sock);
    if (roomCode.isEmpty()) {
        out["ok"] = false;
        out["error"] = "NOT_IN_ROOM";
        return out;
    }

    QJsonArray arr;
    const auto it = m_chat.find(roomCode);
    if (it != m_chat.end()) {
        for (const ChatMessage &m : it.value()) {
            QJsonObject o;
            o["from"] = m.from;
            o["text"] = m.text;
            o["ts"] = m.tsIso;
            arr.append(o);
        }
    }

    out["ok"] = true;
    out["room"] = roomCode;
    out["messages"] = arr;
    return out;
}
