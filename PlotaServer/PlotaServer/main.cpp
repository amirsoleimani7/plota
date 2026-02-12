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
#include "RoomManager.h"
#include "OthelloRoomService.h"
#include "OthelloService.h"


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
    RoomManager roomMgr;
    OthelloRoomService othelloRooms(roomMgr);
    OthelloService othello(othelloRooms);

    QObject::connect(&othelloRooms, &OthelloRoomService::roomStateChanged,
                     [&](const QString &code, const QJsonObject &state) {
                         Q_UNUSED(code);

                         // Send to both sockets in that room (if connected)
                         const RoomManager::Room *r = roomMgr.getRoom(state.value("room").toString());
                         if (!r) return;

                         auto sendTo = [&](QTcpSocket *s) {
                             if (!s) return;
                             QJsonObject msg;
                             msg["type"] = "othello_state";
                             msg["payload"] = state;

                             QByteArray out = QJsonDocument(msg).toJson(QJsonDocument::Compact);
                             out.append('\n');
                             s->write(out);
                         };

                         sendTo(r->black);
                         sendTo(r->white);
                     });


    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();

        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [&auth, &othello, clientSocket]() {
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
                else if (type == "othello_create_room") {
                    reply["type"] = "othello_create_room_result";
                    replyPayload = othello.handleCreateRoom(clientSocket);
                }
                else if (type == "othello_join_room") {
                    reply["type"] = "othello_join_room_result";
                    replyPayload = othello.handleJoinRoom(clientSocket, payload);
                }
                else if (type == "othello_move") {
                    reply["type"] = "othello_move_result";
                    replyPayload = othello.handleMove(clientSocket, payload);
                }
                else if (type == "othello_leave_room") {
                    reply["type"] = "othello_leave_room_result";
                    replyPayload = othello.handleLeaveRoom(clientSocket);
                }
                else if (type == "othello_get_state") {
                    reply["type"] = "othello_state_result";
                    replyPayload = othello.handleGetState(clientSocket);
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

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [&sessions, &othello, clientSocket]() {
                             qDebug() << "Client disconnected";
                             othello.handleDisconnect(clientSocket);   // leaves room if needed
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
