#include "files.h"

files::files(QObject *parent) : QObject(parent)
{
    sClientListPath = QDir::currentPath().append("/common/clientlist");
    sIpListPath = QDir::currentPath().append("/common/srviplist");
    sCertPath = QDir::currentPath().append("/ssl/server.crt");
    sKeyPath = QDir::currentPath().append("/ssl/server.key");
    dToday = QDate::currentDate();
    sLogPath = QDir::currentPath().append("/logs/").append(dToday.toString("dd-MM-yyyy"));

    fClientList = new QFile(this);
    fClientList->setFileName(sClientListPath);
    fCert = new QFile(this);
    fCert->setFileName(sCertPath);
    fKey = new QFile(this);
    fKey->setFileName(sKeyPath);
    fLog = new QFile(this);
    fLog->setFileName(sLogPath);
}

files::~files()
{
    this->closeFiles();
    if (fClientList != 0)
    {delete fClientList;}

    if (fCert != 0)
    {delete fCert;}

    if (fKey != 0)
    {delete fKey;}
}

bool files::openFiles()
{
    if (!fClientList->open(QFile::ReadWrite | QFile::Text))
    {emit this->setStatus(true,fClientList->errorString());return false;}

    if (!fCert->open(QFile::ReadOnly | QFile::Text))
    {emit this->setStatus(true,fCert->errorString());return false;}

    if (!fKey->open(QFile::ReadOnly | QFile::Text))
    {emit this->setStatus(true,fKey->errorString());return false;}

    if (!fLog->exists())
    {if (!fLog->open(QFile::WriteOnly | QFile::Text))
        {emit this->setStatus(true,fLog->errorString());return false;}
    }
    else
    {if (!fLog->open(QFile::Append | QFile::Text))
        {emit this->setStatus(true,fLog->errorString());return false;}
    }
    return true;
}

void files::writeLog(const QString &sLog)
{
    if (dToday!=QDate::currentDate())
    {
        fLog->close();
        dToday = QDate::currentDate();
        sLogPath = QDir::currentPath().append("/logs/").append(dToday.toString("dd-MM-yyyy"));
        fLog->setFileName(sLogPath);
        if (!fLog->open(QFile::WriteOnly | QFile::Text))
        {emit this->setStatus(true,fLog->errorString());return;}
    }

    QTextStream tsOut(fLog);
    tsOut.setCodec("UTF-8");
    tsOut << QTime::currentTime().toString("HH:mm:ss").prepend("[").append("] ").append(sLog).append("\n");
    fLog->flush();
}

void files::closeFiles()
{
    if (fClientList->isOpen())
    {fClientList->close();}

    if (fCert->isOpen())
    {fCert->close();}

    if (fKey->isOpen())
    {fKey->close();}
}

QStringList files::getClientList()
{
    QStringList slClients;

    QString sFline;
    QTextStream tsIn(fClientList);
    tsIn.setCodec("UTF-8");
    do
    {
        sFline = tsIn.readLine();

        if (sFline.isEmpty() || QString::compare(sFline.at(0),"#") == 0)
        {continue;}

        slClients << sFline;
        continue;

    } while (!sFline.isNull());

    return slClients;
}

int files::checkIp(const QString &sIp)
{
    //0-failed to open file, 1-compare successed, 2-compare failed, 3-ip is empty

    if (sIp.isNull() || sIp.isEmpty())
    {return 3;}
    QFile fIpList(sIpListPath);

    if (fIpList.open(QFile::ReadWrite))
    {
        QString sFline;
        QTextStream tsIn(&fIpList);
        tsIn.setCodec("UTF-8");
        do
        {
            sFline = tsIn.readLine();

            if (sFline.isEmpty() || QString::compare(sFline.at(0),"#") == 0)
            {continue;}

            if (QString::compare(sIp,sFline) == 0)
            {fIpList.close(); return 1;}
            continue;

        } while (!sFline.isNull());
    }
    else
    {fIpList.close(); return 0;}

    fIpList.close();

    QString sBackupFilePath = QString(sIpListPath).append(".b");
    this->writeFile(sIp,sBackupFilePath);

    return 2;
}

void files::uploadSuccess(const bool &res)
{
    QString sBackupFilePath = QString(sIpListPath).append(".b");

    if (!QFile::exists(sBackupFilePath))
    {emit this->setStatus(true,"backup file does not exist");return;}

    if (res)
    {QFile::remove(sIpListPath);QFile::rename(sBackupFilePath,sIpListPath); return;}
    else
    {QFile::remove(sBackupFilePath); return;}
}

void files::writeFile(const QString &sOut, const QString &sFileName)
{
    QFile *fOut = new QFile(this);
    QTextStream tsOut(fOut);

    fOut->setFileName(sFileName);
    tsOut.setCodec("UTF-8");
    if (fOut->open(QIODevice::WriteOnly | QIODevice::Text))
    {tsOut << sOut;}
    fOut->flush();
    fOut->close();
    delete fOut;
}

QByteArray files::getCert()
{
    QByteArray baCert = fCert->readAll();
    return baCert;
}

QByteArray files::getKey()
{
    QByteArray baKey = fKey->readAll();
    return baKey;
}
