#include "client.h"

sslClient::sslClient(const int &descriptor, QSqlDatabase *pdb, const QByteArray &baSslCert, const QByteArray &baSslKey, const QStringList &slClients, QObject *parent) : QObject(parent)
{
    sslSocket = new QSslSocket(this);

    connect(sslSocket,SIGNAL(connected()),this,SLOT(socketConnected()));
    connect(sslSocket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    connect(sslSocket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
    connect(sslSocket,SIGNAL(encrypted()),this,SLOT(socketEncrypted()));
    connect(sslSocket,SIGNAL(sslErrors(QList<QSslError>)),this,SLOT(sslError(QList<QSslError>)));

    connect(this,SIGNAL(taskFinished()),this,SLOT(prepareProcess()));

    db=pdb;
    _baSslCert = baSslCert;
    _baSslKey = baSslKey;
    _slClients = slClients;
    socketDescriptor = descriptor;
    dbw = new database(db, this);
    connect(dbw,SIGNAL(sendResult(QByteArray)),this,SLOT(sqlResult(QByteArray)));

    authenticated = false;
}

sslClient::~sslClient()
{
//    sslSocket->deleteLater();
}

void sslClient::setSocket()
{
    if (!sslSocket->supportsSsl())
    {this->socketError("ssl is not supported");return;}

    if (!sslSocket->setSocketDescriptor(socketDescriptor))
    {this->socketError(sslSocket->errorString());return;}

    sslSocket->setProtocol(QSsl::TlsV1_0);

    QSslKey sslKey(_baSslKey, QSsl::Rsa);
    QSslCertificate sslCert(_baSslCert);

    if (sslKey.isNull())
    {this->socketError("ssl key is empty");return;}

    if (sslCert.isNull())
    {this->socketError("ssl certificate is empty");return;}

    sslSocket->setLocalCertificate(sslCert);
    sslSocket->setPrivateKey(sslKey);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    sslSocket->startServerEncryption();

    this->prepareProcess();
}

void sslClient::prepareProcess()
{
    sRequestStamp = "-";
    if (this->getBuffer()!=0)
    {this->socketError("Error receiving data");return;}
//    qDebug() << sRequestStamp << baBuffer;

    QByteArray baTmp;
    for (int iitm=0; iitm<baBuffer.size();)
    {
        baTmp.append(baBuffer.at(iitm));
        baBuffer.remove(iitm,1);
        if (baBuffer.at(iitm) == '\n')
        {baBuffer.remove(iitm,1);break;}
    }

    QStringList slParams = QString(baTmp).split(";;");

    if (slParams.size()!=3)
    {this->sendBuffer("srv;;respond;;1\nerror,input-srvreq");return;}

//    sRequestStamp = slParams.takeAt(0);

    qDebug() << slParams << baBuffer;

    if(QString::compare(slParams.at(0),"srv")==0)
    {
        if(QString::compare(slParams.at(1),"request")==0)
        {this->processServerRequest(this->parseCsv(baBuffer));}
    }
    else if(QString::compare(slParams.at(0),"db")==0)
    {
        if(QString::compare(slParams.at(1),"request")==0)
        {this->processDbRequest(this->parseCsv(baBuffer));}
    }
}

int sslClient::sendBuffer(const QByteArray &baSend)
{
    QByteArray baDataSize = QByteArray::number(baSend.size());
    if (baDataSize.size() > 4096)
    {this->socketError("Request size is too large");return 1;}

    sslSocket->write(sRequestStamp.toStdString().c_str());
    if(!sslSocket->waitForBytesWritten())
    {this->serverTimeout();return 1;}
    if(!sslSocket->waitForReadyRead())
    {this->serverTimeout();return 1;}

    QString sReqStamp = sslSocket->readAll();

    if (QString::compare(sReqStamp,sRequestStamp)!=0)
    {this->socketError("Error synchronizing request stamps");baBuffer.clear();return 1;}

    sslSocket->write(baDataSize);
    if(!sslSocket->waitForBytesWritten())
    {this->serverTimeout();return 1;}
    bytesExpected = 0;
    if(!sslSocket->waitForReadyRead())
    {this->serverTimeout();return 1;}

    bytesExpected = sslSocket->readAll().toLongLong();

    if (bytesExpected != (baSend.size()))
    {this->socketError("Error transmitting data send");baBuffer.clear();return 1;}

    sslSocket->write(baSend);
    if(!sslSocket->waitForBytesWritten())
    {this->serverTimeout();return 1;}

    return 0;
}

int sslClient::getBuffer()
{
    baBuffer.clear();
    if(!sslSocket->waitForReadyRead())
    {this->serverTimeout();return 1;}
    baBuffer = sslSocket->readAll();
    sRequestStamp = baBuffer;
    sslSocket->write(baBuffer);
    if(!sslSocket->waitForBytesWritten())
    {this->serverTimeout();return 1;}

    baBuffer.clear();
    bytesExpected = 0;
    sslSocket->waitForReadyRead(-1);
    baBuffer = sslSocket->readAll();
    bytesExpected = baBuffer.toLongLong();
    sslSocket->write(baBuffer);
    if(!sslSocket->waitForBytesWritten())
    {this->serverTimeout();return 1;}
    baBuffer.clear();

    while (bytesExpected != 0)
    {
        if (!sslSocket->waitForReadyRead(5000))
        {this->socketError("Error transmitting data get");return 1;}

        QByteArray baChunk = sslSocket->read(qMin(bytesExpected,sslSocket->bytesAvailable()));
        baBuffer.append(baChunk);
        bytesExpected -= baChunk.size();

        if (bytesExpected < 0)
        {
            baBuffer.clear();
            bytesExpected=0;
            this->socketError("Error transmitting data chunk");
            return 1;
        }
    }
    return 0;
}

QVector<QStringList> sslClient::parseCsv(QByteArray baCsv)
{
    QString sCsv(baCsv);
    baCsv.clear();

    csvReader *crw = new csvReader;
    return crw->parseCsv(sCsv);
}

void sslClient::processDbRequest(const QVector<QStringList> &vslRequest)
{
    if (!authenticated) {return;}
    if (vslRequest.size()!=1) {this->socketError("Incorrect database request");return;}
    dbw->procQuery(vslRequest.at(0));
}

void sslClient::processServerRequest(const QVector<QStringList> &vslRequest)
{
//    qDebug() << slPurpose.size() << slPurpose;
//    qDebug() << slData.size()<< slData;
    qDebug() << vslRequest;

    if (vslRequest.isEmpty()) {return;}
    if (QString::compare(vslRequest.at(0).at(0),"client-auth")==0)
    {
        sClientName = vslRequest.at(0).at(1);
        emit this->setStatus(true,QString("client ").append(sClientName).append(" is trying to connect"));

        if (!_slClients.contains(sClientName))
        {
            this->sendBuffer("srv;;respond;;1\nerror,auth-clientnotfound");
            authenticated = false;
            this->socketError("client is not found in the client list");
            sslSocket->disconnectFromHost();
        }

        emit this->sConn(socketDescriptor,sslSocket,vslRequest.at(0).at(1));
        emit this->setStatus(true,QString("client ").append(sClientName).append(" verified"));

        this->sendBuffer("srv;;respond;;0\nstart");
        authenticated = true;
        emit this->setStatus(false,"client connected");
    }
    else if (QString::compare(vslRequest.at(0).at(0),"checkonline")==0)
    {this->sendBuffer("srv;;respond;;0\nonline");}
    else if (QString::compare(vslRequest.at(0).at(0),"client-close")==0)
    {
        this->sendBuffer(QByteArray("srv;;respond;;0\nclosed,\"").append(sClientName).append("\""));
        authenticated = false;
        return;
    }

    emit this->setStatus(false,"task done");
    emit this->taskFinished();
}

void sslClient::socketConnected()
{
    emit this->setStatus(false,"client connected event");
}

void sslClient::socketEncrypted()
{
    emit this->setStatus(false,"encrypted connection established");
}

void sslClient::socketDisconnected()
{
//    sslSocket->abort();
    emit this->setStatus(false,QString("client ").append(sClientName).append(" disconnected"));
    emit this->sDisc(socketDescriptor);
    emit this->finished();
}

void sslClient::socketReadyRead()
{
    emit this->setStatus(false,QString("Reading: ").append(QString::number(sslSocket->bytesAvailable())));
}

void sslClient::sslError(const QList<QSslError> &lError)
{
    //client->ignoreSslErrors();
    emit this->setStatus(true,lError.at(lError.size()-1).errorString());
}

void sslClient::socketError(const QString &sErr)
{
    emit this->setStatus(1, sErr);
//    sslSocket->close();
//    sslSocket->deleteLater();
}

void sslClient::serverTimeout()
{
    this->socketError("timeout, bitch");
    sslSocket->close();
}

void sslClient::sqlResult(const QByteArray &baSqlOutput)
{
    this->sendBuffer(baSqlOutput);
    emit this->setStatus(false,"task done");
    emit this->taskFinished();
//    sslSocket->deleteLater();
}

//-------------------------------------------------------------------------------------------------------------
networkStatus::networkStatus(const QString &sIpServer)
{
    uIpsrv = QUrl(sIpServer);
}

networkStatus::~networkStatus()
{
}

void networkStatus::networkIsUp()
{
    QList<QNetworkInterface> lIf = QNetworkInterface::allInterfaces();
    if (lIf.isEmpty())
    {emit this->setStatus(true,"lflist is empty"); return;}

    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkRequest request(uIpsrv);
    request.setRawHeader("User-Agent", "Anonymous");
    rpl = nam->get(request);

    connect(rpl,SIGNAL(readyRead()),this,SLOT(rplReadyRead()));
    connect(rpl,SIGNAL(finished()),this,SLOT(rplFinished()));
    connect(rpl,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(updateDownloadProgress(qint64,qint64)));
    connect(rpl,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(replyError(QNetworkReply::NetworkError)));

    baBuffer.clear();
}

void networkStatus::rplReadyRead()
{
    baBuffer.append(rpl->readAll().simplified());
}

void networkStatus::replyError(const QNetworkReply::NetworkError &neState)
{
    emit this->setStatus(true,QString::number(neState));
}

void networkStatus::updateDownloadProgress(qint64 bytesRead, qint64 bytesTotal)
{
    emit this->setStatus(false,QString::number(bytesRead).append("-").append(QString::number(bytesTotal)));
}

void networkStatus::rplFinished()
{
    if (rpl->error() == QNetworkReply::NoError)
    {emit this->setIp(QString(baBuffer));}
    baBuffer.clear();
    rpl->deleteLater();
}

//-------------------------------------------------------------------------
uploadFile::uploadFile(const QStringList &slFtpData)
{
    _slFtpData = slFtpData;
//    ftpData.clear();
    fIpList = 0;
}

uploadFile::~uploadFile()
{
}

void uploadFile::uploadSettingsFile()
{
    fIpList = new QFile(_slFtpData.at(0));
    if (fIpList->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QUrl uStorage = QUrl(_slFtpData.at(1));
        uStorage.setUserName(_slFtpData.at(2));
        uStorage.setPassword(_slFtpData.at(3));
        uStorage.setPort(_slFtpData.at(4).toInt());

        QNetworkAccessManager* nam = new QNetworkAccessManager();
        rpl = nam->put(QNetworkRequest(uStorage), fIpList);
        connect(rpl,SIGNAL(finished()),this,SLOT(uploadFinished()));
        connect(rpl,SIGNAL(bytesWritten(qint64)),this,SLOT(rplBytesWritten(qint64)));
        connect(rpl,SIGNAL(uploadProgress(qint64,qint64)),this,SLOT(uploadProgress(qint64,qint64)));
        connect(rpl,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(replyError(QNetworkReply::NetworkError)));
    }
}

void uploadFile::uploadProgress(qint64 bytesRead, qint64 bytesTotal)
{
    emit this->setStatus(false,QString::number(bytesRead).append("-").append(QString::number(bytesTotal)));
}

void uploadFile::rplBytesWritten(qint64 bytesWritten)
{
    emit this->setStatus(false,QString::number(bytesWritten));
}

void uploadFile::uploadFinished()
{
    fIpList->close();
    delete fIpList;

    if (rpl->error() == QNetworkReply::NoError)
    {
        emit this->setStatus(false,"upload successful");
        emit this->uploadSuccess(true);
    }
    else
    {
        emit this->setStatus(true,rpl->errorString());
        emit this->uploadSuccess(false);
    }
    rpl->deleteLater();
    emit this->finished();
}

void uploadFile::replyError(const QNetworkReply::NetworkError &neState)
{
    emit this->setStatus(true,QString::number(neState));
//    qDebug() << state;
}

//------------------------------------------------------------------------------------------------------

csvReader::csvReader()
{

}

csvReader::~csvReader()
{

}

QVector<QStringList> csvReader::parseCsv(QString sData)
{
    sData.remove(QRegExp("\r")); //remove all ocurrences of CR (Carriage Return)
    QString sTemp;
    QChar cCurrent;
    QTextStream tsIn(&sData);
    while (!tsIn.atEnd())
    {
        tsIn >> cCurrent;
        if (cCurrent == ',')
        {this->checkString(sTemp, cCurrent);}
        else if (cCurrent == '\n')
        {this->checkString(sTemp, cCurrent);}
        else if (tsIn.atEnd())
        {sTemp.append(cCurrent);this->checkString(sTemp);}
        else
        {sTemp.append(cCurrent);}
    }
    return vslOut;
}

void csvReader::checkString(QString &sTemp, QChar cCurrent)
{
    if(sTemp.count("\"")%2 == 0)
    {
//        if (sTemp.size() == 0 && cCurrent != ',') //problem with line endings
//        {return;}
        if (sTemp.startsWith(QChar('\"')) && sTemp.endsWith(QChar('\"')))
        {
             sTemp.remove(QRegExp("^\""));
             sTemp.remove(QRegExp("\"$"));
        }
        //FIXME: will possibly fail if there are 4 or more reapeating double quotes
        sTemp.replace("\"\"", "\"");
        slOut.append(sTemp);
        if (cCurrent != QChar(','))
        {vslOut.append(slOut); slOut.clear();}
        sTemp.clear();
    }
    else
    {sTemp.append(cCurrent);}
}
