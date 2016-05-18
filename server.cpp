#include "server.h"

sslServer::sslServer(QObject *parent) : QTcpServer(parent)
{
    connect(this,SIGNAL(setStatus(bool,QString)),this,SLOT(logger(bool,QString)));
    connect(this,SIGNAL(ipIsSet(QString)),this,SLOT(checkIp(QString)));
    connect(this,SIGNAL(ipIsSet(QString)),this,SLOT(serverStatus(QString)));

    timerCheck = new QTimer(this);
    connect(timerCheck,SIGNAL(timeout()),this,SLOT(getServerStatus()));
    timerCheck->setInterval(300000);

    fw = new files(this);
    connect(fw,SIGNAL(setStatus(bool,QString)),this,SIGNAL(setStatus(bool,QString)));
    if (!fw->openFiles())
    {delete fw;this->stopServer();}
    this->loadData();
}

sslServer::~sslServer()
{
    this->stopServer();
    delete timerCheck;
    delete db;
    delete fw;
}

void sslServer::setLaunchParameters(const QStringList &slArgs)
{
    Q_UNUSED(slArgs);
    srvMainWindow *smw = new srvMainWindow();
    connect(this,SIGNAL(setStatus(bool,QString)),smw,SLOT(logger(bool,QString)));
    smw->show();

    this->startServer();
    this->setupDatabase(QStringList() << "QPSQL" << "127.0.0.1" << "pharmIPP" << "dbuser" << "z&0fY34*t#u*$s4x" << "5432");
}

void sslServer::setupDatabase(const QStringList &slDbConn)
{
    db = new QSqlDatabase(QSqlDatabase::addDatabase(slDbConn.at(0)));
    db->setConnectOptions();
    db->setHostName(slDbConn.at(1));
    db->setDatabaseName(slDbConn.at(2));
    db->setUserName(slDbConn.at(3));
    db->setPassword(slDbConn.at(4));
    db->setPort(slDbConn.at(5).toInt());
    if (!db->open())
    {this->logger(true,db->lastError().text());}
}

void sslServer::loadData()
{
    baSslCert = fw->getCert();
    baSslKey = fw->getKey();
    slClients = fw->getClientList();
}

void sslServer::getServerStatus()
{
    emit this->setStatus(false,"starting server status get");
    this->getExternalIp("http://muzmarket.kz/utilites/ci.php");
}

void sslServer::startServer()
{
    if (!this->listen(QHostAddress::Any,10742))
    {emit this->setStatus(true,this->errorString());return;}
    this->getServerStatus();
    timerCheck->start();
    return;
}

void sslServer::stopServer()
{
    timerCheck->stop();

    foreach (int iitm,mClients.keys())
    {this->removeClient(iitm);}

    this->close();
}

void sslServer::getExternalIp(const QString &sIsa)
{
    emit this->setStatus(false,"starting external IP get");

    QThread *tNetworkStatus = new QThread(this);
    networkStatus *nsw = new networkStatus(sIsa);
    nsw->moveToThread(tNetworkStatus);
    connect(nsw,SIGNAL(setStatus(bool,QString)),this,SIGNAL(setStatus(bool,QString)));
    connect(tNetworkStatus,SIGNAL(started()),nsw,SLOT(networkIsUp()),Qt::QueuedConnection);
    connect(nsw,SIGNAL(setIp(QString)),this,SLOT(setExternalIp(QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(ipIsSet(QString)),tNetworkStatus,SLOT(quit()));
    connect(tNetworkStatus,SIGNAL(finished()),nsw,SLOT(deleteLater()));
    connect(nsw,SIGNAL(destroyed()),tNetworkStatus,SLOT(deleteLater()));
    tNetworkStatus->start();
}

void sslServer::checkIp(const QString &sExternalIp)
{
    switch (fw->checkIp(sExternalIp))
    {
    case 0:
        emit this->setStatus(false,"operation with serverlist file failed");
        break;
    case 1:
        emit this->setStatus(false,"external IP matched with saved in file");
        break;
    case 2:
        emit this->setStatus(true,QString("new ip is: ").append(sExternalIp));
        this->uploadSettingsFile();
        break;
    case 3:
        emit this->setStatus(false,"external ip is not available");
        break;
    default:
        emit this->setStatus(true,"unknown comparison error");
        break;
    }
}

void sslServer::serverStatus(const QString &sExternalIp)
{
    if (this->isListening())
    {emit this->setStatus(false,this->serverAddress().toString().append(":").append(QString::number(this->serverPort())));}

    QString sIpAddress;
    QList<QHostAddress> lhaIpAddresses = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < lhaIpAddresses.size(); ++i)
    {
        if (lhaIpAddresses.at(i) != QHostAddress::LocalHost &&
            lhaIpAddresses.at(i).toIPv4Address())
        {
            sIpAddress = lhaIpAddresses.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (sIpAddress.isEmpty())
    {sIpAddress = QHostAddress(QHostAddress::LocalHost).toString();}
    emit this->setStatus(false,QString("The server is running on\ninternal IP: %1\nexternal IP: %2\nport: %3\n")
                         .arg(sIpAddress).arg(sExternalIp).arg(QString::number(this->serverPort())));
}

void sslServer::uploadSettingsFile()
{
    emit this->setStatus(false,"starting upload");

    QStringList slFtpData;
    slFtpData << QDir::currentPath().append("/common/srviplist.b") << "ftp://ftp.muzmarket.kz/srviplist"
               << "utilites@muzmarket.kz" << "47$Y@1hj0oo^3" << "21";
    QThread *tUploadFile = new QThread();
    uploadFile *ufw = new uploadFile(slFtpData);
    ufw->moveToThread(tUploadFile);
    connect(ufw,SIGNAL(setStatus(bool,QString)),this,SIGNAL(setStatus(bool,QString)));
    connect(ufw,SIGNAL(uploadSuccess(bool)),fw,SLOT(uploadSuccess(bool)));
    connect(tUploadFile,SIGNAL(started()),ufw,SLOT(uploadSettingsFile()),Qt::QueuedConnection);
    connect(ufw,SIGNAL(finished()),tUploadFile,SLOT(quit()));
    connect(tUploadFile,SIGNAL(finished()),ufw,SLOT(deleteLater()));
    connect(ufw,SIGNAL(destroyed()),tUploadFile,SLOT(deleteLater()));
    tUploadFile->start();
}

void sslServer::setExternalIp(const QString &sEip)
{
    if (sEip.isEmpty())
    {timerCheck->setInterval(20000);}
    else
    {
        timerCheck->setInterval(300000);
        emit this->setStatus(false,QString("external IP is ").append(sEip));
    }
    emit this->ipIsSet(sEip);
}

void sslServer::incomingConnection(qintptr socketDescriptor)
{
    emit this->setStatus(false,QString("incoming connection. descriptor ").append(QString::number(socketDescriptor)));

    QThread *tSslClient = new QThread(this);
    sslClient *scw = new sslClient(socketDescriptor, db, baSslCert, baSslKey, slClients);
    scw->moveToThread(tSslClient);
    connect(scw,SIGNAL(setStatus(bool,QString)),this,SLOT(logger(bool,QString)));
    connect(tSslClient,SIGNAL(started()),scw,SLOT(setSocket()),Qt::QueuedConnection);
    connect(scw,SIGNAL(sConn(int,QSslSocket*,QString)),this,SLOT(addClient(int,QSslSocket*,QString)));
    connect(scw,SIGNAL(sDisc(int)),this,SLOT(removeClient(int)));
    connect(scw,SIGNAL(finished()),tSslClient,SLOT(quit()));
    connect(tSslClient,SIGNAL(finished()),scw,SLOT(deleteLater()));
    connect(scw,SIGNAL(destroyed()),tSslClient,SLOT(deleteLater()));
    tSslClient->start();
}

void sslServer::addClient(const int &descriptor, QSslSocket *s, const QString &sClid)
{
    QMapIterator<int,QPair<QSslSocket *,QString> > iter(mClients);
    while(iter.hasNext())
    {
        iter.next();
        if (QString::compare(mClients.value(iter.key()).second,sClid)==0)
        {this->removeClient(descriptor);}
    }
    mClients[descriptor] = qMakePair(s, sClid);
    emit this->setStatus(false,QString("added ").append(QString::number(descriptor)));
}

void sslServer::removeClient(const int &descriptor)
{
//    if (mClients[descriptor].first->isOpen()) {mClients[descriptor].first->close();}
    mClients.remove(descriptor);
    emit this->setStatus(false,QString("removed ").append(QString::number(descriptor)));
}

void sslServer::logger(const bool &sw, const QString &sLog)
{
    if (sw)
    {fw->writeLog(sLog);}

    fprintf(stdout, "%s\n", sLog.toStdString().c_str());
    fflush(stdout);
}
