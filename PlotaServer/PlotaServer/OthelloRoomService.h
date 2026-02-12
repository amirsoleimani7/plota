#ifndef OTHELLOROOMSERVICE_H
#define OTHELLOROOMSERVICE_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QJsonObject>
#include <QVector>

#include "OthelloLogic.h"

class QTcpSocket;
class RoomManager;

struct ChatMessage {
    QString from;
    QString text;
    QString tsIso;
};

class OthelloRoomService : public QObject
{
    Q_OBJECT
public:
    explicit OthelloRoomService(RoomManager &rooms, QObject *parent = nullptr);

    QString createRoom(QTcpSocket *creator);
    bool joinRoom(const QString &code, QTcpSocket *joiner);
    QString leaveRoom(QTcpSocket *sock);

    QString roomOf(QTcpSocket *sock) const;
    OthelloGame::Cell playerColor(QTcpSocket *sock) const;

    QJsonObject currentStateFor(QTcpSocket *sock, bool includeLegalMoves = true) const;
    QJsonObject tryMove(QTcpSocket *sock, int r, int c);

    // ✅ chat
    QJsonObject broadcastChat(QTcpSocket *sock, const QString &from, const QString &text);
    QJsonObject getChat(QTcpSocket *sock) const;

signals:
    void roomStateChanged(const QString &code, const QJsonObject &state);
    void roomCreated(const QString &code);
    void roomJoined(const QString &code);

private:
    RoomManager &m_rooms;

    QHash<QString, OthelloGame> m_games;

    // ✅ per-room chat storage
    QHash<QString, QVector<ChatMessage>> m_chat; // roomCode -> last N

    void ensureGameExists(const QString &code);
    void startGameIfReady(const QString &code);
    void emitState(const QString &code);
};

#endif // OTHELLOROOMSERVICE_H
