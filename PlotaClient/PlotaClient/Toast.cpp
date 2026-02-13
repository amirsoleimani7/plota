#include "Toast.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QStyleOption>
#include <QPainter>

static QString kindStyle(Toast::Kind k)
{
    switch (k) {
    case Toast::Success: return "background:#3fae66;";   // lighter green
    case Toast::Warning: return "background:#d39e2c;";   // lighter amber
    case Toast::Error:   return "background:#c94a4a;";   // lighter red
    case Toast::Info:
    default:             return "background:#4a5568;";   // lighter gray
    }
}

Toast::Toast(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowFlags(Qt::FramelessWindowHint);
    setObjectName("toast");

    setAttribute(Qt::WA_StyledBackground, true);          // ✅ add this


    // Layout
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 8, 14, 8);

    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignCenter);   // ✅ CENTER TEXT
    m_label->setStyleSheet("background: transparent;");   // ✅ add this

    lay->addWidget(m_label);

    // Shadow (ONLY effect we use)
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0,0,0,120));
    setGraphicsEffect(shadow);

    // Fade using windowOpacity (safe)
    setWindowOpacity(0.0);

    m_fadeIn = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeIn->setDuration(160);
    m_fadeIn->setStartValue(0.0);
    m_fadeIn->setEndValue(1.0);
    m_fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    m_fadeOut = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeOut->setDuration(200);
    m_fadeOut->setStartValue(1.0);
    m_fadeOut->setEndValue(0.0);
    m_fadeOut->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_fadeOut, &QPropertyAnimation::finished, this, &Toast::deleteLater);
}

void Toast::setKind(Kind kind)
{
    QColor bg("#e5e7eb"); // info
    QColor fg("#111827"); // dark text

    if (kind == Success) bg = QColor("#d1fae5");
    else if (kind == Warning) bg = QColor("#fef3c7");
    else if (kind == Error) bg = QColor("#fee2e2");

    // ✅ apply style directly to THIS widget (beats global QWidget rules)
    setStyleSheet(QString(
                      "background:%1;"
                      "border-radius:12px;"
                      "padding:6 12px;"
                      "QLabel{"
                      " color:%2;"
                      " background:transparent;"
                      " font-size:14px;"
                      " font-weight:800;"
                      "}"
                      ).arg(bg.name(), fg.name()));

    update();
}

void Toast::start(int ms)
{
    m_fadeIn->start();

    QTimer::singleShot(ms, this, [this](){
        m_fadeOut->start();
    });
}

void Toast::show(QWidget *parent, const QString &text, Kind kind, int ms)
{
    if (!parent) return;

    auto *t = new Toast(parent);
    t->setKind(kind);
    qDebug() << "TOAST style =" << t->styleSheet();
    qDebug() << "TOAST palette window =" << t->palette().color(QPalette::Window);
    t->m_label->setText(text);

    t->m_label->setStyleSheet("background: transparent; color: #111827;");
    const int margin = 15;
    const int maxW = qMin(350, parent->width() - 2*margin);
    t->setFixedWidth(maxW);
    t->adjustSize();

    // ✅ CENTER TOP
    const int x = (parent->width() - t->width()) / 2;
    const int y = margin;
    t->move(x, y);

    t->QWidget::show();
    t->raise();
    t->start(ms);
}

void Toast::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

