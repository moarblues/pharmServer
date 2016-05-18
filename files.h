#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

class files : public QObject
{
    Q_OBJECT

    QFile *fClientList;
    QFile *fCert;
    QFile *fKey;
    QFile *fLog;

    QString sClientListPath;
    QString sIpListPath;
    QString sCertPath;
    QString sKeyPath;
    QString sLogPath;

    QDate dToday;

    void closeFiles();
    void writeFile(const QString &sOut, const QString &sFileName);

public:
    explicit files(QObject *parent = 0);
    virtual ~files();

    bool openFiles();
    void writeLog(const QString &sLog);

    QStringList getClientList();
    int checkIp(const QString &sIp);

    QByteArray getCert();
    QByteArray getKey();
    
signals:
    void setStatus(const bool &sw, const QString &sStatus);
    
public slots:
    void uploadSuccess(const bool &res);
};

#endif // FILES_H
