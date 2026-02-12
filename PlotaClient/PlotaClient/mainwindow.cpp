#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "OthelloPage.h"
#include "OthelloController.h"

#include <QLineEdit>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ---- UI init ----
    ui->stack->setCurrentWidget(ui->pageLogin);

    ui->leLoginPassword->setEchoMode(QLineEdit::Password);
    ui->leSignupPassword->setEchoMode(QLineEdit::Password);
    ui->leForgotNewPassword->setEchoMode(QLineEdit::Password);

    ui->btnConnect4->setEnabled(false);
    ui->btnConnect4->setText("connect4 (coming soon)");

    setLoginStatus("");
    setSignupStatus("");
    setForgotStatus("");
    setEditProfileStatus("");

    // ✅ 1) networking ONCE
    proto = new ClientProtocol(this);

    connect(proto, &ClientProtocol::connected, this, [=](){
        qDebug() << "Connected to server!";
        setLoginStatus("Connected.");
    });

    connect(proto, &ClientProtocol::disconnected, this, [=](){
        qDebug() << "Disconnected from server!";
        setLoginStatus("Disconnected.");
    });

    connect(proto, &ClientProtocol::socketError, this, [=](const QString &e){
        qDebug() << "Socket error:" << e;
        setLoginStatus("Socket error: " + e);
    });

    connect(proto, &ClientProtocol::badMessageReceived, this, [=](const QByteArray &line){
        qDebug() << "Bad JSON from server:" << line;
    });

    connect(proto, &ClientProtocol::messageReceived, this,
            [=](const QString &type, const QJsonObject &payload){
                qDebug() << "Reply type =" << type << "payload =" << payload;

                if (type == "signup_result") {
                    bool ok = payload.value("ok").toBool(false);
                    setSignupStatus(ok ? "Signup OK. You can login now."
                                       : "Signup failed: " + payload.value("error").toString());
                    if (ok) ui->stack->setCurrentWidget(ui->pageLogin);
                }
                else if (type == "login_result") {
                    bool ok = payload.value("ok").toBool(false);
                    if (ok) {
                        currentUsername = ui->leLoginUsername->text().trimmed();
                        currentName = payload.value("name").toString();

                        setLoginStatus("Login OK. Welcome " + currentName);
                        ui->stack->setCurrentWidget(ui->pageMainMenu);

                        proto->sendMessage("get_profile", QJsonObject{});
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
                        ui->stack->setCurrentWidget(ui->pageLogin);
                    } else {
                        setForgotStatus("Reset failed: " + payload.value("error").toString());
                    }
                }
            });

    // ✅ connect ONCE
    proto->connectToHost("127.0.0.1", 45454);

    // ✅ 2) Othello page/controller AFTER proto exists
    othelloPage = new OthelloPage(this);
    ui->stack->addWidget(othelloPage);
    othelloController = new OthelloController(proto, othelloPage, this);

    connect(othelloPage, &OthelloPage::backToMenuClicked, this, [this]() {
        if (proto) proto->sendMessage("othello_leave_room", QJsonObject{});
        ui->stack->setCurrentWidget(ui->pageMainMenu);
    });

    connect(ui->btnOthello, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(othelloPage);
    });

    // ---- Navigation buttons ----
    connect(ui->btnGoSignup, &QPushButton::clicked, this, [=](){
        setSignupStatus("");
        ui->stack->setCurrentWidget(ui->pageSignup);
    });

    connect(ui->btnBackToLogin, &QPushButton::clicked, this, [=](){
        setLoginStatus("");
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    connect(ui->btnForgot, &QPushButton::clicked, this, [=](){
        setForgotStatus("");
        ui->stack->setCurrentWidget(ui->pageForgot);
    });

    connect(ui->btnBackToLoginFromForgot, &QPushButton::clicked, this, [=](){
        setLoginStatus("");
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    connect(ui->btnLogout, &QPushButton::clicked, this, [=](){
        setLoginStatus("Logged out.");
        ui->stack->setCurrentWidget(ui->pageLogin);
    });

    // ---- Actions: Signup/Login/Forgot ----
    connect(ui->btnSignup, &QPushButton::clicked, this, [=](){
        QJsonObject p;
        p["name"] = ui->leSignupName->text().trimmed();
        p["username"] = ui->leSignupUsername->text().trimmed();
        p["phone"] = ui->leSignupPhone->text().trimmed();
        p["email"] = ui->leSignupEmail->text().trimmed();
        p["password"] = ui->leSignupPassword->text();
        proto->sendMessage("signup", p);
    });

    connect(ui->btnLogin, &QPushButton::clicked, this, [=](){
        QJsonObject p;
        p["username"] = ui->leLoginUsername->text().trimmed();
        p["password"] = ui->leLoginPassword->text();
        proto->sendMessage("login", p);
    });

    connect(ui->btnResetPassword, &QPushButton::clicked, this, [=](){
        QJsonObject p;
        p["username"] = ui->leForgotUsername->text().trimmed();
        p["phone"] = ui->leForgotPhone->text().trimmed();
        p["newPassword"] = ui->leForgotNewPassword->text();
        proto->sendMessage("forgot_password", p);
    });

    connect(ui->btnEditProfile, &QPushButton::clicked, this, [=](){
        setEditProfileStatus("");
        ui->stack->setCurrentWidget(ui->pageEditProfile);
    });

    connect(ui->btnBackToMainMenu, &QPushButton::clicked, this, [=](){
        ui->stack->setCurrentWidget(ui->pageMainMenu);
    });

    connect(ui->btnSaveProfile, &QPushButton::clicked, this, [=](){
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
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setLoginStatus(const QString &msg)
{
    ui->lblLoginStatus->setText(msg);
}

void MainWindow::setSignupStatus(const QString &msg)
{
    ui->lblSignupStatus->setText(msg);
}

void MainWindow::setForgotStatus(const QString &msg)
{
    ui->lblForgotStatus->setText(msg);
}

void MainWindow::setEditProfileStatus(const QString &msg)
{
    ui->lblEditProfileStatus->setText(msg);
}
