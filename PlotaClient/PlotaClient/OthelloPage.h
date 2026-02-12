#ifndef OTHELLOPAGE_H
#define OTHELLOPAGE_H

#include <QWidget>
#include <QVector>
#include <QPoint>

class QLabel;
class QPushButton;
class QLineEdit;
class QTextEdit;
class OthelloBoardWidget;

class OthelloPage : public QWidget
{
    Q_OBJECT
public:
    explicit OthelloPage(QWidget *parent = nullptr);

    void setGameState(const QVector<QVector<int>> &board,
                      const QVector<QPoint> &legalMoves,
                      bool myTurn,
                      bool waiting);

    void setInRoom(bool inRoom);
    OthelloBoardWidget* boardWidget() const { return m_board; }

    // UI setters
    void setStatus(const QString &text);
    void setRoomCode(const QString &code);

    // UI getters
    QString roomCodeInput() const;

    // ✅ chat UI helpers
    void appendChatMessage(const QString &from, const QString &text, const QString &ts);
    void clearChat();

signals:
    void createRoomClicked();
    void joinRoomClicked();
    void leaveRoomClicked();
    void backToMenuClicked();

    // ✅ chat
    void chatSendClicked(const QString &text);

private:
    QLabel *lblTitle = nullptr;
    QLabel *lblStatus = nullptr;

    QPushButton *btnBack = nullptr;
    QPushButton *btnCreate = nullptr;

    QLineEdit *leRoom = nullptr;
    QPushButton *btnJoin = nullptr;

    QPushButton *btnLeave = nullptr;

    OthelloBoardWidget *m_board = nullptr;

    // ✅ chat widgets
    QTextEdit *teChat = nullptr;
    QLineEdit *leChat = nullptr;
    QPushButton *btnSend = nullptr;
};

#endif // OTHELLOPAGE_H
