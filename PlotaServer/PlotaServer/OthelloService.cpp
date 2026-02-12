#include "OthelloService.h"
#include "OthelloRoomService.h"

#include <QTcpSocket>
#include <QJsonDocument>
#include <QDateTime>


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


OthelloService::OthelloService(OthelloRoomService &rooms)
    : m_rooms(rooms)
{
}

QJsonObject OthelloService::handleCreateRoom(QTcpSocket *sock)
{
    QJsonObject out;

    // require logged in? (optional)
    // For now we allow anyone connected. Later you can enforce sessions.

    QString code = m_rooms.createRoom(sock);
    if (code.isEmpty()) {
        out["ok"] = false;
        out["error"] = "CREATE_ROOM_FAILED";
        out["hint"] = "You may already be in a room.";
        return out;
    }

    out["ok"] = true;
    out["room"] = code;
    out["youAre"] = (int)m_rooms.playerColor(sock); // should be Black
    return out;
}

QJsonObject OthelloService::handleJoinRoom(QTcpSocket *sock, const QJsonObject &payload)
{
    QJsonObject out;

    QString code = payload.value("room").toString().trimmed();
    if (code.isEmpty()) {
        out["ok"] = false;
        out["error"] = "EMPTY_ROOM_CODE";
        return out;
    }

    bool ok = m_rooms.joinRoom(code, sock);
    if (!ok) {
        out["ok"] = false;
        out["error"] = "JOIN_FAILED";
        out["hint"] = "Room may not exist, may be full, or you may already be in a room.";
        return out;
    }

    out["ok"] = true;
    out["room"] = code;
    out["youAre"] = (int)m_rooms.playerColor(sock); // should be White
    return out;
}

QJsonObject OthelloService::handleLeaveRoom(QTcpSocket *sock)
{
    QJsonObject out;

    QString code = m_rooms.leaveRoom(sock);
    if (code.isEmpty()) {
        out["ok"] = false;
        out["error"] = "NOT_IN_ROOM";
        return out;
    }

    out["ok"] = true;
    out["room"] = code;
    return out;
}

QJsonObject OthelloService::handleGetState(QTcpSocket *sock)
{
    // OthelloRoomService already returns {ok, error?, state?}
    return m_rooms.currentStateFor(sock, true);
}

QJsonObject OthelloService::handleMove(QTcpSocket *sock, const QJsonObject &payload)
{
    // payload needs {r:int, c:int}
    QJsonObject out;

    if (!payload.contains("r") || !payload.contains("c")) {
        out["ok"] = false;
        out["error"] = "MISSING_ROW_COL";
        return out;
    }

    int r = payload.value("r").toInt(-1);
    int c = payload.value("c").toInt(-1);

    // OthelloRoomService will do bounds + turn + legality checks
    return m_rooms.tryMove(sock, r, c);
}

void OthelloService::handleDisconnect(QTcpSocket *sock)
{
    // Just leave room if in one.
    m_rooms.leaveRoom(sock);
}

QJsonObject OthelloService::handleChatSend(QTcpSocket *sock, const QJsonObject &payload)
{
    QJsonObject reply;

    // Must be in a room (room service knows)
    QString text = payload.value("text").toString().trimmed();

    if (text.isEmpty()) {
        reply["ok"] = false;
        reply["error"] = "EMPTY_MESSAGE";
        return reply;
    }
    if (text.size() > 300) {
        reply["ok"] = false;
        reply["error"] = "MESSAGE_TOO_LONG";
        return reply;
    }

    // Username: if you don't have sessions here yet, use fallback.
    // Better later: inject SessionManager and get real username.
    QString from = payload.value("from").toString().trimmed();
    if (from.isEmpty()) from = "Player";

    // This function will broadcast to both players in that room.
    QJsonObject res = m_rooms.broadcastChat(sock, from, text);

    // res should be like: { ok:true, room:"ABC123" } or { ok:false, error:"NOT_IN_ROOM" }
    return res;
}


