#pragma once
#include <QWidget>

class QLabel;
class QPushButton;
class QProgressBar;

class ConnectionPage : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionPage(QWidget *parent = nullptr);

    void setConnecting(const QString &hostPortText);
    void setFailed(const QString &errorText);

signals:
    void retryClicked();

private:
    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    QLabel *m_error = nullptr;
    QProgressBar *m_progress = nullptr;
    QPushButton *m_retry = nullptr;
};
