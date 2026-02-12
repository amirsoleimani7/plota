#include "OthelloPage.h"
#include "OthelloBoardWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

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

    // top controls
    auto *row1 = new QHBoxLayout();
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

    // board (real widget)
    m_board = new OthelloBoardWidget(this);
    m_board->setMinimumHeight(380);

    root->addWidget(lblTitle);
    root->addWidget(lblStatus);
    root->addLayout(row1);
    root->addLayout(row2);
    root->addWidget(btnLeave);
    root->addWidget(m_board, 1);

    connect(btnCreate, &QPushButton::clicked, this, &OthelloPage::createRoomClicked);
    connect(btnJoin, &QPushButton::clicked, this, &OthelloPage::joinRoomClicked);
    connect(btnLeave, &QPushButton::clicked, this, &OthelloPage::leaveRoomClicked);
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
