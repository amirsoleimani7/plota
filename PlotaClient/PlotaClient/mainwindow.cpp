#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QJsonParseError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stack->setCurrentWidget(ui->pageLogin);
    connect(ui->btnGoSignup, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageSignup);
    });

    connect(ui->btnBackToLogin, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    connect(ui->btnForgot, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageForgot);
    });

    connect(ui->btnBackToLoginFromForgot, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    connect(ui->btnLogout, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, [=](){
        qDebug() << "Connected to server!";

        // 1) signup
        QJsonObject signup;
        signup["type"] = "signup";
        QJsonObject sp;
        sp["name"] = "Alice";
        sp["username"] = "alice";
        sp["phone"] = "09120000000";
        sp["email"] = "alice@example.com";
        sp["passwordHash"] = "TEMP_HASH_123"; // we will hash properly later
        signup["payload"] = sp;
        sendJson(signup);

        QJsonObject signup2 = signup;
        sendJson(signup2);


        // 2) login
        QJsonObject login;
        login["type"] = "login";
        QJsonObject lp;
        lp["username"] = "alice";
        lp["passwordHash"] = "TEMP_HASH_123";
        login["payload"] = lp;
        sendJson(login);

    });


    connect(socket, &QTcpSocket::readyRead, this, [=](){
        while (socket->canReadLine()) {
            QByteArray line = socket->readLine().trimmed();

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                qDebug() << "Bad JSON from server:" << line;
                continue;
            }

            QJsonObject msg = doc.object();
            QString type = msg.value("type").toString();
            QJsonObject payload = msg.value("payload").toObject();

            qDebug() << "Reply type =" << type << "payload =" << payload;
        }
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


// navigation



