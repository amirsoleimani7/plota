#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

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
                QJsonObject reply;
                reply["type"] = "hello_ack";
                QJsonObject replyPayload;
                replyPayload["ok"] = true;
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
