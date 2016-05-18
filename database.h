#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

class database : public QObject
{
    Q_OBJECT

    QSqlDatabase *db;

    QList<QStringList> runQuery(const QString &sQry);
    void qrySelect(const QStringList &slRequest);
    void qrySelectLists(const QStringList &slRequest);
    void qrySelectRetail(const QStringList &slRequest);
    void qrySelectRegistry(const QStringList &slRequest);
    void qryInsert(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria);
    void qryInsertLists(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria);
    void qryInsertGoods(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria);
    QByteArray createCsv(const QList<QStringList> &lslInput);

public:
    explicit database(QSqlDatabase *pdb, QObject *parent = 0);
    virtual ~database();
    void procQuery(const QStringList &slRequest);

signals:
    void sendResult(const QByteArray &baQryRes);
    void setStatus(const bool &sw, const QString &sStatus);
    
public slots:
    
};

#endif // DATABASE_H
