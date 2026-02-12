#ifndef OTHELLOSERVICE_H
#define OTHELLOSERVICE_H

#include <QJsonObject>

class QTcpSocket;
class OthelloRoomService;
class SessionManager;

class OthelloService
{
public:
    explicit OthelloService(OthelloRoomService &rooms, SessionManager &sessions);

    QJsonObject handleCreateRoom(QTcpSocket *sock);
    QJsonObject handleJoinRoom(QTcpSocket *sock, const QJsonObject &payload);
    QJsonObject handleLeaveRoom(QTcpSocket *sock);
    QJsonObject handleGetState(QTcpSocket *sock);
    QJsonObject handleMove(QTcpSocket *sock, const QJsonObject &payload);

    // ✅ chat
    QJsonObject handleChatSend(QTcpSocket *sock, const QJsonObject &payload);
    QJsonObject handleChatGet(QTcpSocket *sock);

    void handleDisconnect(QTcpSocket *sock);

private:
    OthelloRoomService &m_rooms;
    SessionManager &m_sessions;
};

#endif // OTHELLOSERVICE_H
