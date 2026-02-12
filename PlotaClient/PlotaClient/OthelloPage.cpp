#include "OthelloPage.h"
#include "OthelloBoardWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateTime>

OthelloPage::OthelloPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(12);

    lblTitle = new QLabel("Othello", this);
    QFont f = lblTitle->font();
    f.setPointSize(f.pointSize() + 6);
    f.setBold(true);
    lblTitle->setFont(f);

    lblStatus = new QLabel("", this);
    lblStatus->setWordWrap(true);

    // ---- top controls ----
    auto *row1 = new QHBoxLayout();

    btnBack = new QPushButton("Back", this);
    row1->addWidget(btnBack);

    btnCreate = new QPushButton("Create Room", this);
    row1->addWidget(btnCreate);

    row1->addStretch(1);

    auto *row2 = new QHBoxLayout();
    leRoom = new QLineEdit(this);
    leRoom->setPlaceholderText("Room code (e.g. ABC123)");
    btnJoin = new QPushButton("Join", this);
    row2->addWidget(leRoom, 1);
    row2->addWidget(btnJoin);

    btnLeave = new QPushButton("Leave Room", this);
    btnLeave->setEnabled(false);

    // ---- board ----
    m_board = new OthelloBoardWidget(this);
    m_board->setMinimumHeight(380);

    // ---- chat UI ----
    teChat = new QTextEdit(this);
    teChat->setReadOnly(true);
    teChat->setMinimumHeight(140);
    teChat->setPlaceholderText("Chat will appear here...");

    auto *chatRow = new QHBoxLayout();
    leChat = new QLineEdit(this);
    leChat->setPlaceholderText("Type a message...");
    btnSend = new QPushButton("Send", this);

    chatRow->addWidget(leChat, 1);
    chatRow->addWidget(btnSend);

    // ---- add to layout ----
    root->addWidget(lblTitle);
    root->addWidget(lblStatus);
    root->addLayout(row1);
    root->addLayout(row2);
    root->addWidget(btnLeave);

    root->addWidget(m_board, 1);

    root->addWidget(teChat);
    root->addLayout(chatRow);

    // ---- signals ----
    connect(btnCreate, &QPushButton::clicked, this, &OthelloPage::createRoomClicked);
    connect(btnJoin, &QPushButton::clicked, this, &OthelloPage::joinRoomClicked);
    connect(btnLeave, &QPushButton::clicked, this, &OthelloPage::leaveRoomClicked);
    connect(btnBack, &QPushButton::clicked, this, &OthelloPage::backToMenuClicked);

    // send chat on button or Enter key
    auto sendNow = [this]() {
        QString text = leChat->text().trimmed();
        if (text.isEmpty()) return;
        leChat->clear();
        emit chatSendClicked(text);
    };

    connect(btnSend, &QPushButton::clicked, this, sendNow);
    connect(leChat, &QLineEdit::returnPressed, this, sendNow);
}

void OthelloPage::setStatus(const QString &text)
{
    lblStatus->setText(text);
}

void OthelloPage::setRoomCode(const QString &code)
{
    leRoom->setText(code);
}

QString OthelloPage::roomCodeInput() const
{
    return leRoom->text().trimmed();
}

void OthelloPage::setInRoom(bool inRoom)
{
    btnLeave->setEnabled(inRoom);

    // optional: disable create/join while in room
    btnCreate->setEnabled(!inRoom);
    btnJoin->setEnabled(!inRoom);
    leRoom->setEnabled(!inRoom);

    // chat input enabled only when in room
    leChat->setEnabled(inRoom);
    btnSend->setEnabled(inRoom);

    if (!inRoom) {
        clearChat();
    }
}

void OthelloPage::setGameState(const QVector<QVector<int>> &board,
                               const QVector<QPoint> &legalMoves,
                               bool myTurn,
                               bool waiting)
{
    if (!m_board) return;
    m_board->setBoard(board);
    m_board->setLegalMoves(legalMoves);
    m_board->setMyTurn(myTurn);
    m_board->setWaiting(waiting);
}

void OthelloPage::appendChatMessage(const QString &from, const QString &text, const QString &ts)
{
    // simple formatting
    QString line = QString("[%1] %2: %3").arg(ts, from, text);
    teChat->append(line);
}

void OthelloPage::clearChat()
{
    if (teChat) teChat->clear();
}
