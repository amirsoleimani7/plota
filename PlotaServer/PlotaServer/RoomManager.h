#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>

class QTcpSocket;
class RoomManager : public QObject
{
    Q_OBJECT
public:
    explicit RoomManager(QObject *parent = nullptr);

    struct Room {
        QString code;          // room code like "A1B2C3"
        QTcpSocket *black = nullptr; // player 1
        QTcpSocket *white = nullptr; // player 2
        bool started = false;  // becomes true when 2 players are present
    };

    // Create a room. Creator becomes BLACK.
    // Returns room code (empty if failed).
    QString createRoom(QTcpSocket *creator);

    // Join an existing room. Joiner becomes WHITE.
    // Returns true if join succeeded.
    bool joinRoom(const QString &code, QTcpSocket *joiner);

    // Remove a socket from whatever room it is in.
    // Returns the room code if it was in a room, else empty.
    QString leaveRoom(QTcpSocket *sock);

    // If socket is in a room, return its room code; else empty.
    QString roomOf(QTcpSocket *sock) const;

    // Access room by code (nullptr if not found)
    Room* getRoom(const QString &code);
    const Room* getRoom(const QString &code) const;

    // Opponent socket (nullptr if none)
    QTcpSocket* opponentOf(QTcpSocket *sock) const;

    // Is room full? (both players present)
    bool isFull(const QString &code) const;

signals:
    void roomCreated(const QString &code);
    void roomJoined(const QString &code);
    void roomEmptied(const QString &code);   // room removed
    void roomBecameFull(const QString &code);

private:
    QString generateCode() const;
    void removeRoom(const QString &code);

    QHash<QString, Room> m_rooms;                 // code -> Room
    QHash<QTcpSocket*, QString> m_socketToRoom;   // socket -> code
};

#endif // ROOMMANAGER_H
