#include "RoomManager.h"

#include <QTcpSocket>
#include <QRandomGenerator>

RoomManager::RoomManager(QObject *parent)
    : QObject(parent)
{
}

QString RoomManager::generateCode() const
{
    // 6 chars, A-Z + 0-9
    static const QString alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    // (no I, O, 0, 1 to reduce confusion)

    QString code;
    code.reserve(6);

    for (int i = 0; i < 6; ++i) {
        int idx = QRandomGenerator::global()->bounded(alphabet.size());
        code.append(alphabet.at(idx));
    }
    return code;
}

QString RoomManager::createRoom(QTcpSocket *creator)
{
    if (!creator) return "";

    // If creator is already in a room, reject by returning empty.
    if (m_socketToRoom.contains(creator))
        return "";

    // Generate unique code
    QString code;
    for (int tries = 0; tries < 50; ++tries) {
        code = generateCode();
        if (!m_rooms.contains(code))
            break;
    }
    if (code.isEmpty() || m_rooms.contains(code))
        return "";

    Room r;
    r.code = code;
    r.black = creator;
    r.white = nullptr;
    r.started = false;

    m_rooms.insert(code, r);
    m_socketToRoom.insert(creator, code);

    emit roomCreated(code);
    return code;
}

bool RoomManager::joinRoom(const QString &code, QTcpSocket *joiner)
{
    if (!joiner) return false;
    if (code.isEmpty()) return false;

    // If joiner already in a room, reject.
    if (m_socketToRoom.contains(joiner))
        return false;

    auto it = m_rooms.find(code);
    if (it == m_rooms.end())
        return false;

    Room &r = it.value();

    // room already full
    if (r.black && r.white)
        return false;

    // don't let same socket occupy both slots
    if (r.black == joiner)
        return false;

    // joiner becomes white
    r.white = joiner;
    m_socketToRoom.insert(joiner, code);

    emit roomJoined(code);

    // if now full, mark started
    if (r.black && r.white) {
        r.started = true;
        emit roomBecameFull(code);
    }

    return true;
}

QString RoomManager::leaveRoom(QTcpSocket *sock)
{
    if (!sock) return "";

    auto it = m_socketToRoom.find(sock);
    if (it == m_socketToRoom.end())
        return "";

    const QString code = it.value();
    m_socketToRoom.erase(it);

    auto roomIt = m_rooms.find(code);
    if (roomIt == m_rooms.end())
        return code;

    Room &r = roomIt.value();

    if (r.black == sock) r.black = nullptr;
    if (r.white == sock) r.white = nullptr;

    // if both gone => delete room
    if (!r.black && !r.white) {
        removeRoom(code);
    } else {
        // room is no longer "started/full"
        r.started = (r.black && r.white);
    }

    return code;
}

void RoomManager::removeRoom(const QString &code)
{
    m_rooms.remove(code);
    emit roomEmptied(code);
}

QString RoomManager::roomOf(QTcpSocket *sock) const
{
    return m_socketToRoom.value(sock);
}

RoomManager::Room* RoomManager::getRoom(const QString &code)
{
    auto it = m_rooms.find(code);
    if (it == m_rooms.end()) return nullptr;
    return &it.value();
}

const RoomManager::Room* RoomManager::getRoom(const QString &code) const
{
    auto it = m_rooms.find(code);
    if (it == m_rooms.end()) return nullptr;
    return &it.value();
}

QTcpSocket* RoomManager::opponentOf(QTcpSocket *sock) const
{
    const QString code = roomOf(sock);
    if (code.isEmpty()) return nullptr;

    const Room *r = getRoom(code);
    if (!r) return nullptr;

    if (r->black == sock) return r->white;
    if (r->white == sock) return r->black;
    return nullptr;
}

bool RoomManager::isFull(const QString &code) const
{
    const Room *r = getRoom(code);
    if (!r) return false;
    return (r->black && r->white);
}
