#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, [=](){
        qDebug() << "Connected to server!";

        QJsonObject msg;
        msg["type"] = "hello";
        QJsonObject payload;
        payload["user"] = "test";
        msg["payload"] = payload;

        sendJson(msg);
    });


    connect(socket, &QTcpSocket::readyRead, this, [=](){
        QByteArray data = socket->readAll();
        qDebug() << "Received from server:" << data;
    });

    socket->connectToHost("127.0.0.1", 45454);

}

MainWindow::~MainWindow()
{
    delete ui;
}



// helper function for sending the data
void MainWindow::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n'); // newline-delimited JSON
    socket->write(data);
}

