#ifndef OTHELLOSERVICE_H
#define OTHELLOSERVICE_H

#include <QJsonObject>

class QTcpSocket;
class OthelloRoomService;

class OthelloService
{
public:
    explicit OthelloService(OthelloRoomService &rooms);

    // Messages
    QJsonObject handleCreateRoom(QTcpSocket *sock);
    QJsonObject handleJoinRoom(QTcpSocket *sock, const QJsonObject &payload);
    QJsonObject handleLeaveRoom(QTcpSocket *sock);
    QJsonObject handleGetState(QTcpSocket *sock);
    QJsonObject handleMove(QTcpSocket *sock, const QJsonObject &payload);

    // Called by main.cpp on disconnect
    void handleDisconnect(QTcpSocket *sock);

private:
    OthelloRoomService &m_rooms;
};

#endif // OTHELLOSERVICE_H
