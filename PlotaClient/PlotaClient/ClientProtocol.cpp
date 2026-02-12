#include "ClientProtocol.h"

#include <QTcpSocket>
#include <QAbstractSocket>

#include <QJsonDocument>
#include <QJsonParseError>

ClientProtocol::ClientProtocol(QObject *parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &ClientProtocol::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientProtocol::disconnected);

    connect(m_socket, &QTcpSocket::readyRead, this, [this]() { onReadyRead(); });

    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit socketError(m_socket->errorString());
    });
}

void ClientProtocol::connectToHost(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void ClientProtocol::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

bool ClientProtocol::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ClientProtocol::sendMessage(const QString &type, const QJsonObject &payload)
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["type"] = type;
    msg["payload"] = payload;

    QJsonDocument doc(msg);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n'); // newline-delimited JSON
    m_socket->write(data);
}

void ClientProtocol::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit badMessageReceived(line);
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj.value("type").toString();
        QJsonObject payload = obj.value("payload").toObject();

        emit messageReceived(type, payload);
    }
}
