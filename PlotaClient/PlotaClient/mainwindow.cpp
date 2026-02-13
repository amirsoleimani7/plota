#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "OthelloPage.h"
#include "OthelloController.h"
#include "ConnectionPage.h"
#include "StackAnimator.h"

#include <QTimer>
#include <QLineEdit>
#include <QDebug>

void MainWindow::switchPage(QWidget *page, int dir, int ms)
{
    if (!ui || !ui->stack || !page) return;

    if (anim) {
        anim->go(ui->stack, page, static_cast<StackAnimator::Direction>(dir), ms);
    } else {
        ui->stack->setCurrentWidget(page);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << "STAGE 1: setupUi done";

    // Create animator (no logic change)
    anim = new StackAnimator(this);

    // --- Connection Page ---
    connectionPage = new ConnectionPage(this);
    ui->stack->insertWidget(0, connectionPage);

    // show connection page FIRST
    ui->stack->setCurrentWidget(connectionPage);
    connectionPage->setConnecting("127.0.0.1:45454");

    connect(connectionPage, &ConnectionPage::retryClicked, this, [this](){
        if (!connectionPage) return;
        connectionPage->setConnecting("127.0.0.1:45454");
        connectToServer();
    });

    qDebug() << "STAGE 2: connectionPage created";

    // --- Protocol (create ONCE) ---
    proto = new ClientProtocol(this);
    qDebug() << "STAGE 3: proto created";

    // --- Basic UI init ---
    ui->leLoginPassword->setEchoMode(QLineEdit::Password);
    ui->leSignupPassword->setEchoMode(QLineEdit::Password);
    ui->leForgotNewPassword->setEchoMode(QLineEdit::Password);

    ui->btnConnect4->setEnabled(false);
    ui->btnConnect4->setText("connect4 (coming soon)");

    setLoginStatus("");
    setSignupStatus("");
    setForgotStatus("");
    setEditProfileStatus("");

    // --- Protocol signals ---
    connect(proto, &ClientProtocol::connected, this, [this](){
        everConnected = true;
        setLoginStatus("Connected.");
        // Connection -> Login (animate)
        switchPage(ui->pageLogin, StackAnimator::Down);
    });

    connect(proto, &ClientProtocol::disconnected, this, [this](){
        setLoginStatus("Disconnected.");
        if (connectionPage) connectionPage->setFailed("Disconnected from server.");
        // Any page -> Connection (animate)
        switchPage(connectionPage, StackAnimator::Up);
    });

    connect(proto, &ClientProtocol::socketError, this, [this](const QString &e){
        setLoginStatus("Socket error: " + e);
        if (connectionPage) connectionPage->setFailed("Socket error:\n" + e);
        switchPage(connectionPage, StackAnimator::Up);
    });

    connect(proto, &ClientProtocol::badMessageReceived, this, [=](const QByteArray &line){
        qDebug() << "Bad JSON from server:" << line;
    });

    connect(proto, &ClientProtocol::messageReceived, this,
            [this](const QString &type, const QJsonObject &payload){
                qDebug() << "Reply type =" << type << "payload =" << payload;
                if (!ui) return;

                if (type == "signup_result") {
                    bool ok = payload.value("ok").toBool(false);
                    setSignupStatus(ok ? "Signup OK. You can login now."
                                       : "Signup failed: " + payload.value("error").toString());
                    if (ok) switchPage(ui->pageLogin, StackAnimator::Right);
                }
                else if (type == "login_result") {
                    bool ok = payload.value("ok").toBool(false);
                    if (ok) {
                        currentUsername = ui->leLoginUsername->text().trimmed();
                        currentName = payload.value("name").toString();

                        setLoginStatus("Login OK. Welcome " + currentName);
                        switchPage(ui->pageMainMenu, StackAnimator::Up);

                        if (proto) proto->sendMessage("get_profile", QJsonObject{});
                    } else {
                        setLoginStatus("Login failed: " + payload.value("error").toString());
                    }
                }
                else if (type == "profile_result") {
                    bool ok = payload.value("ok").toBool(false);
                    if (!ok) {
                        setEditProfileStatus("Profile load failed: " + payload.value("error").toString());
                    } else {
                        ui->leEditName->setText(payload.value("name").toString());
                        ui->leEditUsername->setText(payload.value("username").toString());
                        ui->leEditPhone->setText(payload.value("phone").toString());
                        ui->leEditEmail->setText(payload.value("email").toString());

                        currentUsername = payload.value("username").toString();
                        currentName = payload.value("name").toString();
                    }
                }
                else if (type == "update_profile_result") {
                    bool ok = payload.value("ok").toBool(false);
                    setEditProfileStatus(ok ? "Profile updated successfully."
                                            : "Update failed: " + payload.value("error").toString());
                    if (ok) ui->leEditPassword->clear();
                }
                else if (type == "forgot_result") {
                    bool ok = payload.value("ok").toBool(false);
                    if (ok) {
                        setForgotStatus("Password updated. Login now.");
                        switchPage(ui->pageLogin, StackAnimator::Up);
                    } else {
                        setForgotStatus("Reset failed: " + payload.value("error").toString());
                    }
                }
            });

    // --- Othello page/controller ---
    othelloPage = new OthelloPage(this);
    ui->stack->addWidget(othelloPage);
    othelloController = new OthelloController(proto, othelloPage, this);

    connect(othelloPage, &OthelloPage::backToMenuClicked, this, [this]() {
        if (proto) proto->sendMessage("othello_leave_room", QJsonObject{});
        switchPage(ui->pageMainMenu, StackAnimator::Right);
    });

    connect(ui->btnOthello, &QPushButton::clicked, this, [this](){
        switchPage(othelloPage, StackAnimator::Left);
    });

    // --- Navigation buttons ---
    connect(ui->btnGoSignup, &QPushButton::clicked, this, [this](){
        setSignupStatus("");
        switchPage(ui->pageSignup, StackAnimator::Left);
    });

    connect(ui->btnBackToLogin, &QPushButton::clicked, this, [this](){
        setLoginStatus("");
        switchPage(ui->pageLogin, StackAnimator::Right);
    });

    connect(ui->btnForgot, &QPushButton::clicked, this, [this](){
        setForgotStatus("");
        switchPage(ui->pageForgot, StackAnimator::Down);
    });

    connect(ui->btnBackToLoginFromForgot, &QPushButton::clicked, this, [this](){
        setLoginStatus("");
        switchPage(ui->pageLogin, StackAnimator::Up);
    });

    connect(ui->btnLogout, &QPushButton::clicked, this, [this](){
        setLoginStatus("Logged out.");
        switchPage(ui->pageLogin, StackAnimator::Down);
    });

    connect(ui->btnEditProfile, &QPushButton::clicked, this, [this](){
        setEditProfileStatus("");
        switchPage(ui->pageEditProfile, StackAnimator::Left);
    });

    connect(ui->btnBackToMainMenu, &QPushButton::clicked, this, [this](){
        switchPage(ui->pageMainMenu, StackAnimator::Right);
    });

    // --- Actions ---
    connect(ui->btnSignup, &QPushButton::clicked, this, [this](){
        if (!proto) return;
        QJsonObject p;
        p["name"] = ui->leSignupName->text().trimmed();
        p["username"] = ui->leSignupUsername->text().trimmed();
        p["phone"] = ui->leSignupPhone->text().trimmed();
        p["email"] = ui->leSignupEmail->text().trimmed();
        p["password"] = ui->leSignupPassword->text();
        proto->sendMessage("signup", p);
    });

    connect(ui->btnLogin, &QPushButton::clicked, this, [this](){
        if (!proto) return;
        QJsonObject p;
        p["username"] = ui->leLoginUsername->text().trimmed();
        p["password"] = ui->leLoginPassword->text();
        proto->sendMessage("login", p);
    });

    connect(ui->btnResetPassword, &QPushButton::clicked, this, [this](){
        if (!proto) return;
        QJsonObject p;
        p["username"] = ui->leForgotUsername->text().trimmed();
        p["phone"] = ui->leForgotPhone->text().trimmed();
        p["newPassword"] = ui->leForgotNewPassword->text();
        proto->sendMessage("forgot_password", p);
    });

    connect(ui->btnSaveProfile, &QPushButton::clicked, this, [this](){
        if (!proto) return;
        if (currentUsername.isEmpty()) {
            setEditProfileStatus("Not logged in.");
            return;
        }
        QJsonObject p;
        p["newName"] = ui->leEditName->text().trimmed();
        p["newPhone"] = ui->leEditPhone->text().trimmed();
        p["newEmail"] = ui->leEditEmail->text().trimmed();
        p["newPassword"] = ui->leEditPassword->text();
        proto->sendMessage("update_profile", p);
    });

    // --- connect (once) ---
    QTimer::singleShot(0, this, [this](){
        connectToServer();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setLoginStatus(const QString &msg)
{
    if (ui) ui->lblLoginStatus->setText(msg);
}

void MainWindow::setSignupStatus(const QString &msg)
{
    if (ui) ui->lblSignupStatus->setText(msg);
}

void MainWindow::setForgotStatus(const QString &msg)
{
    if (ui) ui->lblForgotStatus->setText(msg);
}

void MainWindow::setEditProfileStatus(const QString &msg)
{
    if (ui) ui->lblEditProfileStatus->setText(msg);
}

void MainWindow::connectToServer()
{
    if (!proto) return;
    proto->connectToHost("127.0.0.1", 45454);
}
