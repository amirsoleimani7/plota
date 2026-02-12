#include "OthelloRoomService.h"
#include "RoomManager.h"

#include <QTcpSocket>

OthelloRoomService::OthelloRoomService(RoomManager &rooms, QObject *parent)
    : QObject(parent), m_rooms(rooms)
{
    // When room becomes full, start/reset game
    connect(&m_rooms, &RoomManager::roomBecameFull, this, [this](const QString &code){
        startGameIfReady(code);
    });

    // If a room is deleted, clean its game
    connect(&m_rooms, &RoomManager::roomEmptied, this, [this](const QString &code){
        m_games.remove(code);
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

    // reset game when second player joins
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

    // emit initial state (waiting for second player)
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

    // startGameIfReady will reset and emit state when room becomes full
    // but in case you joined a room that already had both players (shouldn't),
    // we can emit anyway.
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

    // If room still exists, notify remaining player via state update later (network layer)
    // We'll emit a "state changed" with waiting flag.
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

    // move applied => broadcast new state
    emitState(code);

    out["ok"] = true;
    out["state"] = g.toJsonState(true);
    return out;
}
