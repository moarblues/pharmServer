#ifndef SRVMAINWINDOW_H
#define SRVMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class srvMainWindow;
}

class srvMainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit srvMainWindow(QWidget *parent = 0);
    ~srvMainWindow();
    
private:
    Ui::srvMainWindow *ui;

public slots:
    void logger(const bool &sw, const QString &sLine);

private slots:
    void on_btn_getStatus_clicked();
};

#endif // SRVMAINWINDOW_H
