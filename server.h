#ifndef SERVER_H
#define SERVER_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QThread>
#include <QTcpServer>
#include <QTimer>

#include "client.h"
#include "srvmainwindow.h"

class sslServer : public QTcpServer
{
    Q_OBJECT

    QSqlDatabase *db;
    QByteArray baSslCert;
    QByteArray baSslKey;
    QStringList slClients;
    QTimer *timerCheck;

    QMap<int,QPair<QSslSocket *,QString> > mClients;

    QPointer<files> fw;

    void loadData();
    void getExternalIp(const QString &sIsa);

public:
    explicit sslServer(QObject *parent = 0);
    virtual ~sslServer();
    
protected:
    void incomingConnection(qintptr socketDescriptor);

signals:
    void setStatus(bool sw, const QString &sStatus);
    void ipIsSet(const QString &sExternalIp);

private slots:
    void startServer();
    void stopServer();
    void serverStatus(const QString &sExternalIp);
    void checkIp(const QString &sExternalIp);
    void uploadSettingsFile();

public slots:
    void setupDatabase(const QStringList &slDbConn);
    void setLaunchParameters(const QStringList &slArgs);
    void getServerStatus();
    void setExternalIp(const QString &sEip);
    void addClient(const int &descriptor, QSslSocket *s, const QString &sClid);
    void removeClient(const int &descriptor);
    void logger(const bool &sw, const QString &sLog);
};

#endif // SERVER_H
