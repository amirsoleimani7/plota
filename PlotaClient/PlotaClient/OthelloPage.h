#ifndef OTHELLOPAGE_H
#define OTHELLOPAGE_H

#include <QWidget>
#include <QVector>
#include <QPoint>

class QLabel;
class QPushButton;
class QLineEdit;
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

signals:
    void createRoomClicked();
    void joinRoomClicked();
    void leaveRoomClicked();
    void backToMenuClicked();

private:
    QLabel *lblTitle = nullptr;
    QLabel *lblStatus = nullptr;
    QPushButton *btnBack = nullptr;

    QPushButton *btnCreate = nullptr;

    QLineEdit *leRoom = nullptr;
    QPushButton *btnJoin = nullptr;

    QPushButton *btnLeave = nullptr;

    OthelloBoardWidget *m_board = nullptr;
};

#endif // OTHELLOPAGE_H
