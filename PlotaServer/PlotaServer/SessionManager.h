#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QTcpSocket>
#include <QHash>
#include <QString>

class SessionManager
{
public:
    // Bind a socket to a username (after login success)
    void bind(QTcpSocket* socket, const QString& username);

    // Remove session (on disconnect)
    void unbind(QTcpSocket* socket);

    // Get username for socket (empty if not logged in)
    QString username(QTcpSocket* socket) const;

private:
    QHash<QTcpSocket*, QString> m_socketUser;
};

#endif // SESSIONMANAGER_H
