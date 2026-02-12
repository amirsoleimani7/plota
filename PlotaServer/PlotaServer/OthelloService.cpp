#include "OthelloService.h"
#include "OthelloRoomService.h"
#include "SessionManager.h"

#include <QTcpSocket>

OthelloService::OthelloService(OthelloRoomService &rooms, SessionManager &sessions)
    : m_rooms(rooms)
    , m_sessions(sessions)
{
}

QJsonObject OthelloService::handleCreateRoom(QTcpSocket *sock)
{
    QJsonObject out;

    QString code = m_rooms.createRoom(sock);
    if (code.isEmpty()) {
        out["ok"] = false;
        out["error"] = "CREATE_ROOM_FAILED";
        out["hint"] = "You may already be in a room.";
        return out;
    }

    out["ok"] = true;
    out["room"] = code;
    out["youAre"] = (int)m_rooms.playerColor(sock);
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
    out["youAre"] = (int)m_rooms.playerColor(sock);
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
    return m_rooms.currentStateFor(sock, true);
}

QJsonObject OthelloService::handleMove(QTcpSocket *sock, const QJsonObject &payload)
{
    QJsonObject out;

    if (!payload.contains("r") || !payload.contains("c")) {
        out["ok"] = false;
        out["error"] = "MISSING_ROW_COL";
        return out;
    }

    int r = payload.value("r").toInt(-1);
    int c = payload.value("c").toInt(-1);

    return m_rooms.tryMove(sock, r, c);
}

// ===============================
// ✅ CHAT
// ===============================
QJsonObject OthelloService::handleChatSend(QTcpSocket *sock, const QJsonObject &payload)
{
    QJsonObject reply;

    QString username = m_sessions.username(sock);
    qDebug() << "CHAT username=" << username;

    if (username.isEmpty()) {
        reply["ok"] = false;
        reply["error"] = "NOT_LOGGED_IN";
        return reply;
    }

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

    // Broadcast (and store in memory) inside room service
    return m_rooms.broadcastChat(sock, username, text);
}

QJsonObject OthelloService::handleChatGet(QTcpSocket *sock)
{
    QJsonObject reply;

    QString username = m_sessions.username(sock);
    if (username.isEmpty()) {
        reply["ok"] = false;
        reply["error"] = "NOT_LOGGED_IN";
        return reply;
    }

    return m_rooms.getChat(sock);
}

void OthelloService::handleDisconnect(QTcpSocket *sock)
{
    m_rooms.leaveRoom(sock);
}
