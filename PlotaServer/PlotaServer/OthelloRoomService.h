#ifndef OTHELLOROOMSERVICE_H
#define OTHELLOROOMSERVICE_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QJsonObject>

#include "OthelloLogic.h"

class QTcpSocket;
class RoomManager;

class OthelloRoomService : public QObject
{
    Q_OBJECT
public:
    explicit OthelloRoomService(RoomManager &rooms, QObject *parent = nullptr);

    // Create room: creator becomes BLACK
    // returns code or empty if failed
    QString createRoom(QTcpSocket *creator);

    // Join room: joiner becomes WHITE
    bool joinRoom(const QString &code, QTcpSocket *joiner);

    // Leave room
    QString leaveRoom(QTcpSocket *sock);

    // Is socket in a room?
    QString roomOf(QTcpSocket *sock) const;

    // What color is this socket in its room?
    // returns Empty if not in room or not assigned
    OthelloGame::Cell playerColor(QTcpSocket *sock) const;

    // Get current game state for socket's room
    QJsonObject currentStateFor(QTcpSocket *sock, bool includeLegalMoves = true) const;

    // Attempt a move from a socket (row,col)
    // returns reply payload object {ok:true/false, error?:..., state?:...}
    QJsonObject tryMove(QTcpSocket *sock, int r, int c);

signals:
    // Fired when a room's state changes and should be broadcast to both players
    void roomStateChanged(const QString &code, const QJsonObject &state);

    // Fired when room is created/joined etc. (useful later)
    void roomCreated(const QString &code);
    void roomJoined(const QString &code);

private:
    RoomManager &m_rooms;

    // roomCode -> game
    QHash<QString, OthelloGame> m_games;

    void ensureGameExists(const QString &code);
    void startGameIfReady(const QString &code);

    // broadcast latest state to both players
    void emitState(const QString &code);
};

#endif // OTHELLOROOMSERVICE_H
