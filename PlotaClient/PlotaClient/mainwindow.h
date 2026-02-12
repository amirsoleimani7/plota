#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>



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
    QTcpSocket *socket;
    void sendJson(const QJsonObject &obj);

    void setLoginStatus(const QString &msg);
    void setSignupStatus(const QString &msg);
    void setForgotStatus(const QString &msg);


};
#endif // MAINWINDOW_H
