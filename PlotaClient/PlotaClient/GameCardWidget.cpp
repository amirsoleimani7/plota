#include "GameCardWidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPixmap>
#include <QStyle>
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>

static QPixmap roundedTopCorners(const QPixmap &src, int w, int h, int radius)
{
    if (src.isNull() || w <= 0 || h <= 0) return QPixmap();

    // banner-like crop (fills area)
    QPixmap scaled = src.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap out(w, h);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Clip only top corners rounded, bottom corners square
    QPainterPath path;
    path.moveTo(0, radius);
    path.quadTo(0, 0, radius, 0);
    path.lineTo(w - radius, 0);
    path.quadTo(w, 0, w, radius);
    path.lineTo(w, h);
    path.lineTo(0, h);
    path.closeSubpath();

    p.setClipPath(path);

    // center the scaled image
    int x = (scaled.width() - w) / 2;
    int y = (scaled.height() - h) / 2;
    p.drawPixmap(-x, -y, scaled);

    return out;
}

GameCardWidget::GameCardWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("GameCard");
    setCursor(Qt::PointingHandCursor);

    // Card size like test.png
    setFixedWidth(320);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Shadow (QSS can't do real shadow)
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 10);
    shadow->setColor(QColor(0, 0, 0, 180));
    setGraphicsEffect(shadow);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_img = new QLabel(this);
    m_img->setObjectName("GameCardImage");
    m_img->setFixedHeight(170);      // ✅ correct image height
    m_img->setAlignment(Qt::AlignCenter);
    m_img->setScaledContents(false); // ✅ we scale ourselves (better)

    auto *body = new QWidget(this);
    body->setObjectName("GameCardBody");

    auto *bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(16, 14, 16, 16);
    bodyLay->setSpacing(8);

    m_title = new QLabel("Title", body);
    m_title->setObjectName("GameCardTitle");

    m_sub = new QLabel("Subtitle", body);
    m_sub->setObjectName("GameCardSubtitle");
    m_sub->setWordWrap(true);

    bodyLay->addWidget(m_title);
    bodyLay->addWidget(m_sub);

    root->addWidget(m_img);
    root->addWidget(body);

    updateHover(false);
}

void GameCardWidget::setImage(const QString &resourcePath)
{
    m_imgPath = resourcePath;
    updateImagePixmap();
}

void GameCardWidget::updateImagePixmap()
{
    if (!m_img) return;

    QPixmap px(m_imgPath);
    if (px.isNull()) {
        m_img->clear();
        return;
    }

    // Apply rounded top corners + banner crop like test.png
    const int w = m_img->width();
    const int h = m_img->height();
    const int r = 16;

    m_img->setPixmap(roundedTopCorners(px, w, h, r));
}

void GameCardWidget::setTitle(const QString &t)    { if (m_title) m_title->setText(t); }
void GameCardWidget::setSubtitle(const QString &t) { if (m_sub)   m_sub->setText(t); }

void GameCardWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        emit clicked();
    QFrame::mousePressEvent(e);
}

void GameCardWidget::enterEvent(QEnterEvent *e)
{
    updateHover(true);
    QFrame::enterEvent(e);
}

void GameCardWidget::leaveEvent(QEvent *e)
{
    updateHover(false);
    QFrame::leaveEvent(e);
}

void GameCardWidget::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    updateImagePixmap(); // keep crop correct on resize
}

void GameCardWidget::updateHover(bool on)
{
    setProperty("hover", on);
    style()->unpolish(this);
    style()->polish(this);
    update();
}
