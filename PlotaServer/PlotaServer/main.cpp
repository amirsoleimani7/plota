#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpServer server;

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "Client connected from:"
                 << clientSocket->peerAddress().toString();

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [=]() {
            QByteArray data = clientSocket->readAll();
            qDebug() << "Received from client:" << data;
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
