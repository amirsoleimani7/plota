#include "OthelloService.h"
#include "OthelloRoomService.h"

#include <QTcpSocket>

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
