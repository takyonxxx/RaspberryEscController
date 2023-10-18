#include <QCoreApplication>
#include <esccontroller.h>

int main(int argc, char *argv[])
{   
    QCoreApplication app(argc, argv);
    EscController controller;
    return app.exec();
}
