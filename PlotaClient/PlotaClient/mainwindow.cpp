#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "OthelloPage.h"
#include "OthelloController.h"
#include "ConnectionPage.h"
#include "StackAnimator.h"

#include "ButtonFx.h"
#include <QPushButton>

#include "Toast.h"

#include <QTimer>
#include <QLineEdit>
#include <QDebug>
#include <QIcon>
#include <QPixmap>

#include "GameCardWidget.h"
#include <QVBoxLayout>

// ----------------------------
// Animation page switching
// ----------------------------
void MainWindow::switchPage(QWidget *page, int dir, int ms)
{
    if (!ui || !ui->stack || !page) return;

    if (anim) {
        anim->go(ui->stack, page, static_cast<StackAnimator::Direction>(dir), ms);
    } else {
        ui->stack->setCurrentWidget(page);
    }
}

// ----------------------------
// Toast helper
// ----------------------------
static void alarmToast(QWidget *parent, const QString &msg, Toast::Kind kind, int ms = 2500)
{
    const QString t = msg.trimmed();
    if (t.isEmpty()) return;
    Toast::show(parent, t, kind, ms);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (!m_loginLogo.isNull() && ui->lblLoginLogo) {
        ui->lblLoginLogo->setPixmap(
            m_loginLogo.scaled(
                ui->lblLoginLogo->size(),
                Qt::KeepAspectRatioByExpanding,   // 🔥 this is the key
                Qt::SmoothTransformation
                )
            );
    }
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_loginLogo = QPixmap(":/assets/bg-remove.png");

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
        alarmToast(this, "Reconnecting…", Toast::Info, 1200);
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

    // keep the old connect4 button disabled (even though we won't show it on menu)
    ui->btnConnect4->setEnabled(false);
    ui->btnConnect4->setText("connect4 (coming soon)");

    setLoginStatus("");
    setSignupStatus("");
    setForgotStatus("");
    setEditProfileStatus("");

    // ----------------------------
    // ✅ Resource icons (from resources.qrc)
    // ----------------------------
    {
        const int w = qMax(320, int(this->width() * 0.40));
        ui->btnOthello->setMinimumWidth(w);
        ui->btnConnect4->setMinimumWidth(w);
        ui->btnEditProfile->setMinimumWidth(w);
        ui->btnLogout->setMinimumWidth(w);

        ui->btnOthello->setMinimumHeight(64);
        ui->btnConnect4->setMinimumHeight(64);
        ui->btnEditProfile->setMinimumHeight(64);
        ui->btnLogout->setMinimumHeight(52);

        ui->btnOthello->setIcon(QIcon(":/assets/othello-logo.png"));
        ui->btnConnect4->setIcon(QIcon(":/assets/connect4-logo.png"));
        // ui->btnEditProfile->setIcon(QIcon(":/assets/checkers-logo.png"));

        const QSize iconSz(34, 34);
        ui->btnOthello->setIconSize(iconSz);
        ui->btnConnect4->setIconSize(iconSz);
        ui->btnEditProfile->setIconSize(iconSz);
    }

    // --- Protocol signals ---
    connect(proto, &ClientProtocol::connected, this, [this](){
        everConnected = true;
        setLoginStatus("Connected.");
        switchPage(ui->pageLogin, StackAnimator::Down);
    });

    connect(proto, &ClientProtocol::disconnected, this, [this](){
        setLoginStatus("Disconnected.");
        if (connectionPage) connectionPage->setFailed("Disconnected from server.");
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

    // ----------------------------
    // ✅ Main menu: 3 cards (Othello, Connect4, Checkers)
    // ✅ Keep EditProfile + Logout as normal buttons
    // ----------------------------
    {
        // Cards replace these buttons:
        ui->btnOthello->hide();
        ui->btnConnect4->hide();

        // Keep these as normal buttons:
        ui->btnEditProfile->show();
        ui->btnLogout->show();

        // Create / reuse layout on pageMainMenu
        QLayout *existing = ui->pageMainMenu->layout();
        QVBoxLayout *menuLay = qobject_cast<QVBoxLayout*>(existing);

        if (!menuLay) {
            menuLay = new QVBoxLayout(ui->pageMainMenu);
            menuLay->setContentsMargins(0, 0, 0, 0);
        }

        // If designer already had items in this layout, you may see duplicates.
        // This code assumes pageMainMenu is mostly empty except buttons.
        menuLay->setAlignment(Qt::AlignCenter);
        menuLay->setSpacing(18);

        // Card 1: Othello (clickable)
        auto *cardOthello = new GameCardWidget(ui->pageMainMenu);
        cardOthello->setImage(":/assets/othello-logo.png");
        cardOthello->setTitle("Othello");
        cardOthello->setSubtitle("Play Othello online against others.");
        menuLay->addWidget(cardOthello);

        connect(cardOthello, &GameCardWidget::clicked, this, [this](){
            switchPage(othelloPage, StackAnimator::Left);
        });

        // Card 2: Connect 4 (coming soon)
        auto *cardConnect4 = new GameCardWidget(ui->pageMainMenu);
        cardConnect4->setImage(":/assets/connect4-logo.png");
        cardConnect4->setTitle("Connect 4");
        cardConnect4->setSubtitle("Coming soon...");
        cardConnect4->setEnabled(false);
        menuLay->addWidget(cardConnect4);

        // Card 3: Checkers (coming soon)
        auto *cardCheckers = new GameCardWidget(ui->pageMainMenu);
        cardCheckers->setImage(":/assets/checkers-logo.png");
        cardCheckers->setTitle("Checkers");
        cardCheckers->setSubtitle("Coming soon...");
        cardCheckers->setEnabled(false);
        menuLay->addWidget(cardCheckers);

        // Put normal buttons below cards (same width feel)
        ui->btnEditProfile->setMinimumWidth(320);
        ui->btnLogout->setMinimumWidth(320);

        menuLay->addWidget(ui->btnEditProfile);
        menuLay->addWidget(ui->btnLogout);
    }

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

    // Button click fx
    const auto buttons = this->findChildren<QPushButton*>();
    for (auto *b : buttons) ButtonFx::install(b);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ----------------------------
// Status -> Toast mapping (same as your logic)
// ----------------------------
void MainWindow::setLoginStatus(const QString &msg)
{
    if (ui) ui->lblLoginStatus->setText(msg);

    const QString t = msg.trimmed();
    if (t.isEmpty()) return;

    if (t.startsWith("Login failed", Qt::CaseInsensitive) ||
        t.startsWith("Socket error", Qt::CaseInsensitive) ||
        t.startsWith("Disconnected", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Error);
    } else if (t.startsWith("Login OK", Qt::CaseInsensitive) ||
               t.startsWith("Connected", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Success);
    } else {
        alarmToast(this, t, Toast::Info);
    }
}

void MainWindow::setSignupStatus(const QString &msg)
{
    if (ui) ui->lblSignupStatus->setText(msg);

    const QString t = msg.trimmed();
    if (t.isEmpty()) return;

    if (t.contains("failed", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Error);
    } else if (t.contains("OK", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Success);
    } else {
        alarmToast(this, t, Toast::Info);
    }
}

void MainWindow::setForgotStatus(const QString &msg)
{
    if (ui) ui->lblForgotStatus->setText(msg);

    const QString t = msg.trimmed();
    if (t.isEmpty()) return;

    if (t.contains("failed", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Error);
    } else if (t.contains("updated", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Success);
    } else {
        alarmToast(this, t, Toast::Info);
    }
}

void MainWindow::setEditProfileStatus(const QString &msg)
{
    if (ui) ui->lblEditProfileStatus->setText(msg);

    const QString t = msg.trimmed();
    if (t.isEmpty()) return;

    if (t.contains("failed", Qt::CaseInsensitive) ||
        t.contains("error", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Error);
    } else if (t.contains("success", Qt::CaseInsensitive) ||
               t.contains("updated", Qt::CaseInsensitive)) {
        alarmToast(this, t, Toast::Success);
    } else {
        alarmToast(this, t, Toast::Info);
    }
}

void MainWindow::connectToServer()
{
    if (!proto) return;
    proto->connectToHost("127.0.0.1", 45454);
}

