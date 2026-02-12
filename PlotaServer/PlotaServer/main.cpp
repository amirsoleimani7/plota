#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <QSqlDatabase>

#include "Database.h"
#include "SessionManager.h"
#include "AuthService.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QSqlDatabase db = Database::initDatabase();
    if (!db.isOpen()) {
        qDebug() << "Database is not open. Exiting.";
        return -1;
    }

    QTcpServer server;
    SessionManager sessions;
    AuthService auth(db, sessions);

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();

        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [&auth, clientSocket]() {
            while (clientSocket->canReadLine()) {
                QByteArray line = clientSocket->readLine().trimmed();

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(line, &err);

                if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                    qDebug() << "Invalid JSON:" << line;
                    continue;
                }

                QJsonObject msg = doc.object();
                QString type = msg.value("type").toString();
                QJsonObject payload = msg.value("payload").toObject();

                qDebug() << "JSON type =" << type << "payload =" << payload;

                // reply example
                // ---- handle message types here ----
                QJsonObject reply;
                QJsonObject replyPayload;

                if (type == "hello") {
                    reply["type"] = "hello_ack";
                    replyPayload["ok"] = true;
                }
                else if (type == "login") {
                    reply["type"] = "login_result";
                    replyPayload = auth.handleLogin(clientSocket, payload);
                }
                else if (type == "signup") {
                    reply["type"] = "signup_result";
                    replyPayload = auth.handleSignup(payload);
                }
                else if (type == "get_profile") {
                    reply["type"] = "profile_result";
                    replyPayload = auth.handleGetProfile(clientSocket);
                }
                else if (type == "update_profile") {
                    reply["type"] = "update_profile_result";
                    replyPayload = auth.handleUpdateProfile(clientSocket, payload);
                }
                else if (type == "forgot_password") {
                    reply["type"] = "forgot_result";
                    replyPayload = auth.handleForgotPassword(payload);
                }
                else {
                    reply["type"] = "error";
                    replyPayload["ok"] = false;
                    replyPayload["error"] = "UNKNOWN_TYPE";
                }

                reply["payload"] = replyPayload;

                QByteArray out = QJsonDocument(reply).toJson(QJsonDocument::Compact);
                out.append('\n');
                clientSocket->write(out);
            }
        });

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [&sessions, clientSocket]() {
            qDebug() << "Client disconnected";
            sessions.unbind(clientSocket);
            clientSocket->deleteLater();
        });
    });

    if (!server.listen(QHostAddress::Any, 45454)) {
        qDebug() << "Server failed to start!";
        return -1;
    }

    qDebug() << "Server listening on port 45454...";

    return a.exec();
}
