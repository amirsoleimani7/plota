#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>

#include "ClientProtocol.h"

class OthelloPage;
class OthelloController;
class ConnectionPage;
class StackAnimator;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // who is logged in
    QString currentUsername;
    QString currentName;

    Ui::MainWindow *ui = nullptr;
    ClientProtocol *proto = nullptr;

    OthelloPage *othelloPage = nullptr;
    OthelloController *othelloController = nullptr;

    ConnectionPage *connectionPage = nullptr;
    StackAnimator *anim = nullptr;

    bool everConnected = false;

    void connectToServer();

    void setLoginStatus(const QString &msg);
    void setSignupStatus(const QString &msg);
    void setForgotStatus(const QString &msg);
    void setEditProfileStatus(const QString &msg);

    // Animation-aware page switch (NO logic change, just replaces setCurrentWidget)
    void switchPage(QWidget *page, int dir /* StackAnimator::Direction */, int ms = 220);
};

#endif // MAINWINDOW_H
