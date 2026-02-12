#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QJsonObject>
#include <QSqlDatabase>

class QTcpSocket;
class SessionManager;

class AuthService
{
public:
    explicit AuthService(QSqlDatabase &db, SessionManager &sessions);

    // Returns a reply payload object; caller sets reply["type"] appropriately
    QJsonObject handleLogin(QTcpSocket *socket, const QJsonObject &payload);
    QJsonObject handleSignup(const QJsonObject &payload);
    QJsonObject handleGetProfile(QTcpSocket *socket);
    QJsonObject handleUpdateProfile(QTcpSocket *socket, const QJsonObject &payload);
    QJsonObject handleForgotPassword(const QJsonObject &payload);

private:
    QSqlDatabase &m_db;
    SessionManager &m_sessions;
};

#endif // AUTHSERVICE_H
