#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

#include "ClientProtocol.h"

class OthelloPage;
class OthelloController;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    QString currentUsername;   // who is logged in
    QString currentName;
    void setEditProfileStatus(const QString &msg);


    Ui::MainWindow *ui;
    ClientProtocol *proto = nullptr;
    OthelloPage *othelloPage = nullptr;
    OthelloController *othelloController = nullptr;


    void setLoginStatus(const QString &msg);
    void setSignupStatus(const QString &msg);
    void setForgotStatus(const QString &msg);


};
#endif // MAINWINDOW_H
