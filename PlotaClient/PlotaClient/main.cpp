#include "mainwindow.h"

#include <QApplication>
#include <QFile>

static void applyStyle()
{
    QFile f(":/style/assets/style.qss");
    if (f.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    applyStyle();

    MainWindow w;
    w.show();
    return a.exec();
}
