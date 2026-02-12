#ifndef OTHELLOCONTROLLER_H
#define OTHELLOCONTROLLER_H

#include <QObject>
#include <QJsonObject>

class ClientProtocol;
class OthelloPage;

class OthelloController : public QObject
{
    Q_OBJECT
public:
    OthelloController(ClientProtocol *proto,
                      OthelloPage *page,
                      QObject *parent = nullptr);

private:
    void connectUi();
    void connectProtocol();
    void applyState(const QJsonObject &state);

    void handleMessage(const QString &type, const QJsonObject &payload);

    ClientProtocol *m_proto = nullptr;
    OthelloPage *m_page = nullptr;

    QString m_room;
    int m_myColor = 0; // 1 black, 2 white
};

#endif // OTHELLOCONTROLLER_H
