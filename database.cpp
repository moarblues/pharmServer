#include "database.h"

database::database(QSqlDatabase *pdb, QObject *parent) : QObject(parent)
{
    db=pdb;
}

database::~database()
{
//    delete db;
}

QList<QStringList> database::runQuery(const QString &sQry)
{
    qDebug() << sQry;

    QList<QStringList> lslReuslt;
    QStringList slQueryStatus;
    QSqlQuery *qry = new QSqlQuery(*db);

    if (!db->isOpen())
    {
        emit this->setStatus(true,db->lastError().text());
        slQueryStatus << "1" << db->lastError().text();
        lslReuslt.append(slQueryStatus);
        lslReuslt.append(QStringList());
        return lslReuslt;
    }

    if(!qry->prepare(sQry))
    {
        emit this->setStatus(true,qry->lastError().text());
        slQueryStatus << "1" << db->lastError().text();
        lslReuslt.append(slQueryStatus);
        lslReuslt.append(QStringList());
        return lslReuslt;
    }

    if(!qry->exec(sQry))
    {
        emit this->setStatus(true,qry->lastError().text());
        slQueryStatus << "1" << db->lastError().text();
        lslReuslt.append(slQueryStatus);
        lslReuslt.append(QStringList());
        return lslReuslt;
    }

    slQueryStatus << "0" << "success";
    lslReuslt.append(slQueryStatus);

    while (qry->next())
    {
        QStringList slSqlOutput;
        for (int jCol = 0; jCol < qry->record().count(); ++jCol)
        {slSqlOutput.append(qry->value(jCol).toString());}
        lslReuslt.append(slSqlOutput);
    }
    delete qry;
    emit this->setStatus(false,"query done");
//    qDebug() << baSqlOutput;

//    db->close();
//    QSqlDatabase::removeDatabase(connParams.at(1));
//    qDebug() << db.isOpen() << sqlOutput;
//    lbaReuslt.append(baSqlOutput);
    return lslReuslt;
}

void database::qrySelect(const QStringList &slRequest)
{
    if (QString::compare(slRequest.at(1),"lists") == 0)
    {this->qrySelectLists(slRequest);return;}
    else if (QString::compare(slRequest.at(1),"retail") == 0)
    {this->qrySelectRetail(slRequest);}
    else if (QString::compare(slRequest.at(1),"registry") == 0)
    {this->qrySelectRegistry(slRequest);}
}

void database::qrySelectLists(const QStringList &slRequest)
{
    QString sQry;
    QByteArray baPurpose;

    if (QString::compare(slRequest.at(2),"depots") == 0)
    {baPurpose = QByteArray("db;;depotlist;;"); sQry = "SELECT retail.depots.depot_alias FROM retail.depots";}
    else if (QString::compare(slRequest.at(2),"producttypes") == 0)
    {baPurpose = QByteArray("db;;producttypeslist;;");
        sQry = "SELECT registry.ref_product_types.type_alias FROM registry.ref_product_types";}
    else if (QString::compare(slRequest.at(2),"names") == 0)
    {
        baPurpose = QByteArray("db;;namelist;;");
        sQry.append("SELECT public.product_names.name_code, public.product_names.product_name FROM public.product_names ");
        if(!slRequest.at(3).isEmpty())
        {sQry.append("WHERE product_name ILIKE '%").append(slRequest.at(3)).append("%'");}
    }
    else if (QString::compare(slRequest.at(2),"manufactorers") == 0)
    {
        baPurpose = QByteArray("db;;manufactorerlist;;");
        sQry.append("SELECT public.manufactorers.manufactorer_code, public.manufactorers.manufactorer_name FROM public.manufactorers ");
        if(!slRequest.at(3).isEmpty())
        {
            if(!slRequest.at(3).isEmpty())
            {sQry.append("WHERE manufactorer_name ILIKE '%").append(slRequest.at(3)).append("%'");}
        }
    }
    else if (QString::compare(slRequest.at(2),"units") == 0)
    {
        baPurpose = QByteArray("db;;unitlist;;");
        sQry.append("SELECT public.units.unit_code, public.units.unit_name FROM public.units ");
        if(!slRequest.at(3).isEmpty())
        {
            if(!slRequest.at(3).isEmpty())
            {sQry.append("WHERE unit_name ILIKE '%").append(slRequest.at(3)).append("%'");}
        }
    }
    else if (QString::compare(slRequest.at(2),"products") == 0)
    {
        baPurpose = QByteArray("db;;productlist;;");
        sQry.append("SELECT * FROM retail.view_products ");
        QStringList slCriteria = slRequest.at(3).split("::");

        if (!slCriteria.isEmpty())
        {
            sQry.append("WHERE ");
            bool addedCondition = false;
            if (slCriteria.size()>=1)
            {
                if(!slCriteria.at(0).isEmpty())
                {sQry.append("product_name ILIKE '%").append(slCriteria.at(0)).append("%'");addedCondition=true;}
            }
            if(slCriteria.size()>=2)
            {
                if(!slCriteria.at(1).isEmpty())
                {
                    if (addedCondition) {sQry.append(" AND ");}
                    sQry.append("product_manufactorer_name ILIKE '%").append(slCriteria.at(1)).append("%'");
                    addedCondition=true;
                }
            }
            if(slCriteria.size()>=3)
            {
                if(!slCriteria.at(2).isEmpty())
                {
                    if (addedCondition) {sQry.append(" AND ");}
                    sQry.append("product_unit_name ILIKE '%").append(slCriteria.at(2)).append("%'");
                    addedCondition=true;
                }
            }
            if(slCriteria.size()>=4)
            {
                if(!slCriteria.at(3).isEmpty())
                {
                    if (addedCondition) {sQry.append(" AND ");}
                    sQry.append("barcode = '").append(slCriteria.at(3)).append("'");
                    addedCondition=true;
                }
            }
        }
    }

    QList<QStringList> lslResult = this->runQuery(sQry);

    if(lslResult.size()<2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    QStringList slQueryStatus = lslResult.takeAt(0);

    if(slQueryStatus.size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    emit this->sendResult(baPurpose.append(slQueryStatus.at(0)).append("\n")
                          .append(this->createCsv(lslResult)));

//    emit this->sendResult(baPurpose.append());
}

void database::qrySelectRetail(const QStringList &slRequest)
{
    if (slRequest.size()!=4)
    {
        emit this->setStatus(true,"invalid input data");
        emit this->sendResult(QByteArray("db;;error;;1\nНеверные входные данные"));
        return;
    }

    QByteArray baResult;
    QString sSelect("SELECT ");
    if (QString::compare(slRequest.at(3),"all")==0)
    {sSelect.append("* ");}


}

void database::qrySelectRegistry(const QStringList &slRequest)
{
//    Q_UNUSED(slCriteria);
//    qDebug() << slData;
    if (slRequest.size()!=4)
    {
        emit this->setStatus(true,"invalid input data");
        emit this->sendResult(QByteArray("db;;error;;1\nНеверные входные данные"));
        return;
    }
    QByteArray baResult;
    QString sSelect("SELECT ");
    if (QString::compare(slRequest.at(3),"all")==0)
    {sSelect.append("* ");}

    QString sFrom("FROM ");
    QString sRegTable("view_");
    QString sRegType;
    QString sTableHeaders;
    QList<QStringList> lslRegType;
    lslRegType = this->runQuery(QString("SELECT registry.ref_product_types.type_view_name_id, "
                                         "registry.ref_product_types.type_table_headers "
                                         "FROM registry.ref_product_types "
                                         "WHERE registry.ref_product_types.type_alias='")
                              .append(slRequest.at(2)).append("'"));

//    qDebug() << lslRegType;

    if(lslRegType.size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if(lslRegType.at(0).size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if (QString::compare(lslRegType.at(0).at(0),"1")==0)
    {
        emit this->setStatus(true,"error selecting registry view");
        emit this->sendResult(QByteArray("db;;error;;1\nОшибка выбора вида реестра"));
        return;
    }

    if(lslRegType.at(1).size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    sRegType = lslRegType.at(1).at(0);
    sTableHeaders = lslRegType.at(1).at(1);

    sRegTable.append(sRegType);
    lslRegType = this->runQuery(QString("SELECT 1 FROM pg_catalog.pg_class c "
                                        "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
                                        "WHERE n.nspname = 'registry' "
                                        "AND c.relname = '").append(sRegTable).append("'"));

    if(lslRegType.size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if(lslRegType.at(0).size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if (QString::compare(lslRegType.at(0).at(0),"1")==0)
    {
        emit this->setStatus(true,"error selecting registry view");
        emit this->sendResult(QByteArray("db;;error;;1\nОшибка выбора вида реестра"));
        return;
    }

    if(lslRegType.at(1).size()!=1)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if (QString::compare(lslRegType.at(1).at(0),"1")!=0)
    {
        emit this->setStatus(true,"registry view not found");
        emit this->sendResult(QByteArray("db;;error;;1\nНе найден вид реестра"));
        return;
    }
    sRegTable.prepend("registry.").append(" ");
    sFrom.append(sRegTable);

    QString sWhere;
//    if(QString::compare(slRequest.at(3),"all")!=0)
//    {
//        sWhere = "WHERE ";
//    }
//    emit this->sendResult(baResult);

    QList<QStringList> lslResult = this->runQuery(sSelect.append(sFrom).append(sWhere));

    if(lslResult.size()<2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    QStringList slQueryStatus = lslResult.takeAt(0);

    if(slQueryStatus.size()!=2)
    {emit this->sendResult(QByteArray("db;;error;;1\nОшибка структуры результата"));return;}

    if (QString::compare(slQueryStatus.at(0),"0")!=0)
    {
        emit this->setStatus(true,"error selecting registry");
        emit this->sendResult(QByteArray("db;;error;;1\nОшибка выбора реестра"));
        return;
    }

    baResult.append("db;;registrylist::").append(sRegType).append(";;").append(slQueryStatus.at(0)).append("\n");
    baResult.append(sTableHeaders.toStdString().c_str()).append("\n");
    emit this->sendResult(baResult.append(this->createCsv(lslResult)));
}

void database::qryInsert(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria)
{
    if (QString::compare(slPurpose.at(2),"lists") == 0)
    {this->qryInsertLists(slPurpose,slData,slCriteria);return;}
    if (QString::compare(slPurpose.at(2),"goods") == 0)
    {this->qryInsertGoods(slPurpose,slData,slCriteria);return;}
//    this->qrySelectGoods();
}

void database::qryInsertLists(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria)
{
    QByteArray baPurpose;
    QByteArray baResult;
    QString sReturning;
    QString sQry("INSERT INTO ");
    if(QString::compare(slPurpose.at(3),"names") == 0)
    {
        baPurpose = QByteArray("db--insertednames--");
        sQry.append("public.product_names (product_name) "); sReturning = "name_code, product_name";
    }
    else if(QString::compare(slPurpose.at(3),"manufactorers") == 0)
    {
        baPurpose = QByteArray("db--insertedmanufactorers--");
        sQry.append("public.manufactorers (manufactorer_name) "); sReturning = "manufactorer_code, manufactorer_name";
    }
    else if(QString::compare(slPurpose.at(3),"products") == 0)
    {
        baPurpose = QByteArray("db--insertedproducts--");
        sQry.append("public.products (name_code, manufactorer_code, unit_code, barcode) "); sReturning = "product_code";
    }

    sQry.append("VALUES ('");

    for (int iitm=0; iitm<slData.size(); ++iitm)
    {
        sQry.append(slData.at(iitm));
        if(iitm < slData.size()-1)
        {sQry.append("', '");}
    }
    sQry.append("')  RETURNING ").append(sReturning);

//    baResult = baPurpose.append(this->runQuery(sQry));
    emit this->sendResult(baResult);
}

void database::qryInsertGoods(const QStringList &slPurpose, const QStringList &slData, const QStringList &slCriteria)
{
    QByteArray baResult;
    QString sQry("INSERT INTO ");
    sQry.append("public.ipp_goods ");
}

QByteArray database::createCsv(const QList<QStringList> &lslInput)
{
    QByteArray baOutput;

    for (int irow=0; irow < lslInput.size(); ++irow)
    {
        for (int jcol=0; jcol < lslInput.at(irow).size(); ++jcol)
        {
            bool dqEnclose=false;
            QString sTmp;
            foreach (QChar cStrItm, lslInput.at(irow).at(jcol))
            {
                if (cStrItm == '"') {sTmp.append('"'); dqEnclose=true;}
                else if (cStrItm == ',') {dqEnclose=true;}
                else if (cStrItm == '\n') {dqEnclose=true;}
                else if (cStrItm == '\r') {cStrItm = '\n';dqEnclose=true;}
                sTmp.append(cStrItm);
            }
            if (dqEnclose) {sTmp.prepend("\""); sTmp.append("\"");}
            baOutput.append(sTmp);
            if (jcol != lslInput.at(irow).size()-1) {baOutput.append(",");}
            else {baOutput.append("\n");}
        }
    }
    if (baOutput.endsWith("\n"))
    {baOutput.remove(baOutput.size()-1,1);}
    return baOutput;
}

void database::procQuery(const QStringList &slRequest)
{
    if (QString::compare(slRequest.at(0),"select") == 0)
    {this->qrySelect(slRequest);}
//    else if (QString::compare(slRequest.at(0),"insert") == 0)
//    {this->qryInsert(slRequest);}
//    else if (QString::compare(taskList.at(1),"update") == 0)
//    {this->qryUpdate(taskList);}
    else
    {}
}
