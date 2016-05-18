#include "srvmainwindow.h"
#include "ui_srvmainwindow.h"

srvMainWindow::srvMainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::srvMainWindow)
{
    ui->setupUi(this);
}

srvMainWindow::~srvMainWindow()
{
    delete ui;
}

void srvMainWindow::logger(const bool &sw, const QString &sLine)
{
    Q_UNUSED(sw);
//    if (sw) {sw=false;}
    ui->lW_log->addItem(sLine);
}

void srvMainWindow::on_btn_getStatus_clicked()
{
//    if (!sslsrv->isListening())
//    {this->logger(true,sslsrv->errorString());return;}
//    sslsrv->getServerStatus();
    return;
}
