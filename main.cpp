#include <QApplication>
#include "Mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QIcon icon(":/images/icon.png");
    app.setWindowIcon(icon);

    MainWindow window;
    window.setWindowTitle("Simulation de Voitures");
    window.resize(1000, 1000);
    window.show();

    return app.exec();
}
