#ifndef CLIENT_H
#define CLIENT_H

#include <QPointer>
#include <QSslKey>
#include <QObject>
#include <QSslSocket>
#include <QUrl>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkInterface>

#include "database.h"
#include "files.h"

class sslClient : public QObject
{
    Q_OBJECT

    QSslSocket *sslSocket;
    QSqlDatabase *db;
    database *dbw;
    QByteArray _baSslCert;
    QByteArray _baSslKey;
    QStringList _slClients;
    QString sClientName;
    QString sRequestStamp;
    int socketDescriptor;
    bool authenticated;

    qint64 bytesExpected;
    QByteArray baBuffer;

    int sendBuffer(const QByteArray &baSend);
    void processDbRequest(const QVector<QStringList> &vslRequest);
    void processServerRequest(const QVector<QStringList> &vslRequest);

public:
    explicit sslClient(const int &descriptor, QSqlDatabase *pdb, const QByteArray &baSslCert, const QByteArray &baSslKey, const QStringList &slClients, QObject *parent = 0);
    virtual ~sslClient();

signals:
    void setStatus(const bool &sw,const QString &sStatus);
    void sConn(const int &descriptor, QSslSocket *s, const QString &sClid);
    void sDisc(const int &descriptor);
    void finished();
    void taskFinished();

private slots:
    void socketError(const QString &sErr);
    void socketConnected();
    void socketEncrypted();
    void socketDisconnected();
    void socketReadyRead();
    int getBuffer();
    void prepareProcess();
    void serverTimeout();
    
public slots:
    void setSocket();
    void sslError(const QList<QSslError> &lError);
    void sqlResult(const QByteArray &baSqlOutput);
    QVector<QStringList> parseCsv(QByteArray baCsv);
    
};

class networkStatus : public QObject
{
    Q_OBJECT

    QUrl uIpsrv;
    QNetworkReply *rpl;
    QByteArray baBuffer;

public:
    explicit networkStatus(const QString &sIpServer);
    virtual ~networkStatus();

signals:
    void setIp(QString sServerAddress);
    void setStatus(bool sw,QString sStatus);

private slots:
    void replyError(const QNetworkReply::NetworkError &neState);
    void updateDownloadProgress(qint64 bytesRead, qint64 bytesTotal);
    void rplFinished();
    void rplReadyRead();

public slots:
    void networkIsUp();

};

class uploadFile : public QObject
{
    Q_OBJECT

    QStringList _slFtpData;
    QFile *fIpList;
    QNetworkReply *rpl;

public:
    explicit uploadFile(const QStringList &slFtpData);
    virtual ~uploadFile();

signals:
    void setStatus(bool sw,const QString &sStatus);
    void uploadSuccess(bool res);
    void finished();

private slots:
    void replyError(const QNetworkReply::NetworkError &neState);
    void uploadProgress(qint64 bytesRead, qint64 bytesTotal);
    void rplBytesWritten(qint64 bytesWritten);
    void uploadFinished();

public slots:
    void uploadSettingsFile();

};

class csvReader : public QObject
{
    Q_OBJECT

    QVector<QStringList> vslOut;
    QStringList slOut;

public:
    explicit csvReader();
    virtual ~csvReader();

private slots:
    void checkString(QString &sTemp, QChar cCurrent = 0);

public slots:
    QVector<QStringList> parseCsv(QString sData);
};

//------------------------------------------------------------------------------------------------------

#endif // CLIENT_H
