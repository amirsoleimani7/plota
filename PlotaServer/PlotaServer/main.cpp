#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>


// json file send and recive
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>


//in-memory user store
#include <QMap>



struct User {
    QString name;
    QString username;
    QString phone;
    QString email;
    QString passwordHash; // we’ll hash later
};

QMap<QString, User> users; // key = username


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpServer server;

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [=]() {
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
                else if (type == "signup") {
                    reply["type"] = "signup_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString name     = payload.value("name").toString().trimmed();
                    QString phone    = payload.value("phone").toString().trimmed();
                    QString email    = payload.value("email").toString().trimmed();
                    QString passHash = payload.value("passwordHash").toString();

                    if (username.isEmpty() || passHash.isEmpty()) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "EMPTY_USERNAME_OR_PASSWORD";
                    }
                    else if (users.contains(username)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "USERNAME_TAKEN";
                    }
                    else {
                        User u;
                        u.username = username;
                        u.name = name;
                        u.phone = phone;
                        u.email = email;
                        u.passwordHash = passHash;

                        users.insert(username, u);

                        replyPayload["ok"] = true;
                    }
                }
                else if (type == "login") {
                    reply["type"] = "login_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString passHash = payload.value("passwordHash").toString();

                    if (!users.contains(username)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "NO_SUCH_USER";
                    }
                    else if (users[username].passwordHash != passHash) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "WRONG_PASSWORD";
                    }
                    else {
                        replyPayload["ok"] = true;
                        replyPayload["name"] = users[username].name;
                    }
                }
                else if (type == "forgot_password") {
                    reply["type"] = "forgot_result";

                    QString username = payload.value("username").toString().trimmed();
                    QString phone    = payload.value("phone").toString().trimmed();
                    QString newHash  = payload.value("newPasswordHash").toString();

                    if (!users.contains(username)) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "NO_SUCH_USER";
                    }
                    else if (users[username].phone != phone) {
                        replyPayload["ok"] = false;
                        replyPayload["error"] = "PHONE_MISMATCH";
                    }
                    else {
                        users[username].passwordHash = newHash;
                        replyPayload["ok"] = true;
                    }
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

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [=]() {
            qDebug() << "Client disconnected";
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
