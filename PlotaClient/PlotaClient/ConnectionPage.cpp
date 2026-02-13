#include "ConnectionPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

ConnectionPage::ConnectionPage(QWidget *parent) : QWidget(parent)
{
    setObjectName("ConnectionPage");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(14);
    root->addStretch();

    m_title = new QLabel("Connecting…", this);
    m_title->setObjectName("TitleLabel");
    m_title->setAlignment(Qt::AlignHCenter);

    m_subtitle = new QLabel("", this);
    m_subtitle->setObjectName("SubtleLabel");
    m_subtitle->setAlignment(Qt::AlignHCenter);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 0); // indefinite
    m_progress->setFixedHeight(10);
    m_progress->setTextVisible(false);

    m_error = new QLabel("", this);
    m_error->setObjectName("ErrorLabel");
    m_error->setWordWrap(true);
    m_error->setAlignment(Qt::AlignHCenter);
    m_error->hide();

    m_retry = new QPushButton("Retry", this);
    m_retry->setObjectName("PrimaryButton");
    m_retry->setFixedWidth(160);
    m_retry->hide();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_retry);
    btnRow->addStretch();

    root->addWidget(m_title);
    root->addWidget(m_subtitle);
    root->addSpacing(8);
    root->addWidget(m_progress);
    root->addSpacing(6);
    root->addWidget(m_error);
    root->addLayout(btnRow);

    root->addStretch();

    connect(m_retry, &QPushButton::clicked, this, &ConnectionPage::retryClicked);
}

void ConnectionPage::setConnecting(const QString &hostPortText)
{
    m_title->setText("Connecting…");
    m_subtitle->setText(hostPortText);
    m_error->hide();
    m_retry->hide();
    m_progress->show();
}

void ConnectionPage::setFailed(const QString &errorText)
{
    m_title->setText("Connection failed");
    m_subtitle->setText("Check the server and try again.");
    m_error->setText(errorText);
    m_error->show();
    m_progress->hide();
    m_retry->show();
}
