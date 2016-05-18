#include "server.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    sslServer s;
    s.setLaunchParameters(a.arguments());
    
    return a.exec();
}
