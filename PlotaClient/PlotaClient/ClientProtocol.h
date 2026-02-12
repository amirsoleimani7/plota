#ifndef CLIENTPROTOCOL_H
#define CLIENTPROTOCOL_H

#include <QObject>
#include <QJsonObject>

class QTcpSocket;

class ClientProtocol : public QObject
{
    Q_OBJECT
public:
    explicit ClientProtocol(QObject *parent = nullptr);

    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();

    bool isConnected() const;

    void sendMessage(const QString &type, const QJsonObject &payload);

signals:
    void connected();
    void disconnected();
    void socketError(const QString &errorText);

    void messageReceived(const QString &type, const QJsonObject &payload);
    void badMessageReceived(const QByteArray &rawLine);

private:
    QTcpSocket *m_socket = nullptr;

    void onReadyRead();
};

#endif // CLIENTPROTOCOL_H
