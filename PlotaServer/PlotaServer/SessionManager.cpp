#include "SessionManager.h"

void SessionManager::bind(QTcpSocket* socket, const QString& username)
{
    m_socketUser[socket] = username;
}

void SessionManager::unbind(QTcpSocket* socket)
{
    m_socketUser.remove(socket);
}

QString SessionManager::username(QTcpSocket* socket) const
{
    return m_socketUser.value(socket);
}
