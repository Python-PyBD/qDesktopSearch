#include "wsearch.h"
#include "str.h"
#include "lucy.h"
#include "searcher.h"
#include "ui_wsearch.h"
#include "wsearchresultmodel.h"
#include <QPair>
#include <QList>
#include "wsearchitemdelegate.h"

wSearch::wSearch(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::wSearch),
    m_pIndexer(nullptr),
    m_pLog(nullptr)
{
    ui->setupUi(this);

    {
        wsearchresultModel* pWsearchresultModel = new wsearchresultModel(new searcher(), this->ui->tableView);
        this->ui->tableView->setModel(pWsearchresultModel);
        this->ui->tableView->setItemDelegate(new wsearchitemdelegate(this->ui->tableView));
    }

    on_eSearchTerm_textChanged("");
}

wSearch::~wSearch()
{
    delete ui;
}

void wSearch::on_eSearchTerm_textChanged(const QString &arg1)
{
    this->ui->commandLinkButtonSearch->setEnabled(!str::isempty(arg1));
}

void wSearch::on_commandLinkButtonSearch_clicked()
{
    QTime t;
    t.start();
    QPair<QString, QString> pair;
    pair.first  = this->ui->eSearchTerm->text();
    pair.second = QString::fromStdWString(FIELDNAME_FULLTEXT);
    QList<QPair<QString, QString>> lpSearchInputs;
    lpSearchInputs.append(pair);

    wsearchresultModel* pModel = (wsearchresultModel*)this->ui->tableView->model();
    int hits = (pModel)->search(lpSearchInputs);
    wsearchresultModel* pModel2 = new wsearchresultModel(nullptr, nullptr);
    //TODO: do it better (emit dataChanged?)
    this->ui->tableView->setModel(pModel2);
    this->ui->tableView->setModel(pModel);
    delete pModel2;
    pModel2 = nullptr;
    this->ui->lHitCount->setText(QString::number(hits)+" hits found in "+logger::t_elapsed(t.elapsed()));
}

void wSearch::on_tableView_clicked(const QModelIndex &index)
{

}

void wSearch::on_tableView_doubleClicked(const QModelIndex &index)
{

}

void wSearch::on_tableView_customContextMenuRequested(const QPoint &pos)
{

}

void wSearch::setEnv(logger* pLog, indexer* pIndexer)
{
    this->m_pLog        = pLog;
    this->m_pIndexer    = pIndexer;
    ((wsearchresultModel*)this->ui->tableView->model())->setEnv(pLog, pIndexer);
}
