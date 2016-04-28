#include "indexerworker.h"
#include "indexerthread.h"
#include "str.h"
#include <QTime>
#include <QThread>
#include <QDebug>
#include <QDirIterator>

indexerWorker::indexerWorker(QString sDir2Index, logger* pLogger, jvm* pJvm/*, lucyindexer* pLucyIndexer*/, QObject *parent) : QObject(parent),
    m_pLogger(pLogger), m_pJvm(pJvm), m_sDir2Index(sDir2Index), m_iIndexedFiles(0), m_iIndexingTime(0), m_iFoundFiles(0), m_pLucyIndexer(nullptr)
{

}

void indexerWorker::doWork()
{
    openIndex();

    //ATTENZIONE: 'BlockingQueuedConnection' is important here, but works only if target is in another thread!
    connect(this,SIGNAL(getMetaContents(QString,QMap<QString, QStringList>*)),this->m_pJvm,SLOT(getMetaContents(QString,QMap<QString, QStringList>*)), Qt::BlockingQueuedConnection);

    t.start();
    int iLoop = 0;
    while(true)
    {
        iLoop++;
        m_iIndexingTime     = 0;
        m_iFoundFiles       = 0;
        m_iIndexedFiles     = 0;
        int iDeletedFiles   = 0;

        {   //idx
            if(QThread::currentThread()->isInterruptionRequested()) { finishIndexing(true); return; }
            dir(m_sDir2Index, iLoop);
        }
        {   //opt
            if(QThread::currentThread()->isInterruptionRequested()) { finishIndexing(true); return; }
            if (this->m_pLucyIndexer)this->m_pLucyIndexer->onIndexerThreadFinished(true);
        }
        {   //del
            if(QThread::currentThread()->isInterruptionRequested()) { finishIndexing(true); return; }
            iDeletedFiles = delDeletedFiles();
        }
        if(iDeletedFiles > 999)
        {   //opt again
            if(QThread::currentThread()->isInterruptionRequested()) { finishIndexing(true); return; }
            if (this->m_pLucyIndexer)this->m_pLucyIndexer->onIndexerThreadFinished(true);
        }

        m_iIndexingTime = t.elapsed();
        m_pLogger->inf("an indexer thread finished. found#:"+QString::number(getNrOfFoundFiles())+" indexed#:"+QString::number(getNrOfIndexedFiles())+" NrOfFilesInIdx:"+QString::number(getNrOfFilesInIndex())+" deleteds:"+QString::number(iDeletedFiles)+" time:"+logger::t_elapsed(getIndexTime())+"  dir:"+getIndexedDir()+" loop:"+QString::number(iLoop));

        for(int i=0;i<3333;i++)
        {
            if(QThread::currentThread()->isInterruptionRequested()) { finishIndexing(true); return; }
            QThread::msleep(99/*milliseconds*/);
        }

        t.restart();
    }

    emit finished();
}

void indexerWorker::dir(QString sDir, int iLoop)
{
    //is recursive!
    QString sSuffix = (sDir.endsWith(QDir::separator()) || sDir.endsWith('/')) ? QString("") : QDir::separator();
    QDirIterator it(sDir+sSuffix, QStringList() << "*", QDir::Files | QDir::Dirs | QDir::NoDotDot | QDir::NoDot, QDirIterator::Subdirectories);
    bool bFound=false;
    while (it.hasNext())
    {
        if(QThread::currentThread()->isInterruptionRequested())
        {
            return;
        }

        QString sEntry(it.next());
        sEntry = str::normalizePath(sEntry, false);
        QFileInfo finfo(sEntry);
        if(finfo.isFile())
        {
            bFound = true;
            file(sEntry, finfo, iLoop);
        }
    }
    if(!bFound)
    {
        m_pLogger->wrn("no file found in '"+sDir+"'");
    }
}

void indexerWorker::file(QString sAbsPathName, QFileInfo finfo, int iLoop)
{
    {   //do not index your own index
        sAbsPathName = str::normalizePath(sAbsPathName, false);
        if(sAbsPathName.indexOf("/idx/") > 1)
        {
            return;
        }
    }

    m_iFoundFiles++;

    if(QThread::currentThread()->isInterruptionRequested())
    {
        return;
    }

    if(this->m_pLucyIndexer->isIndexed(sAbsPathName, finfo))
    {
        return;
    }

    if(QThread::currentThread()->isInterruptionRequested())
    {
        return;
    }

    bool bIsBigFile = false;
    {   //if(file>xxxMB)->return;  //because the jvm has to work on the ui thread
        qint64 iSizeInBytes = finfo.size();
        qint64 iSizeInMB    = iSizeInBytes / 1048576;          //1 MB = 1024 KB = 1048576 Byte
        if(iSizeInMB > 99)
        {
            //if(iLoop < 2)
            {   //wrn only in the first loop
                this->m_pLogger->wrn("file too big to be indexed("+QString::number(iSizeInMB)+"MB) fn:"+sAbsPathName);
            }
            bIsBigFile = true;
        }
    }

    QMap<QString, QStringList>* pMetaContents = new QMap<QString, QStringList>();
    //http://stackoverflow.com/questions/19113719/jni-findclass-function-returns-null
    //http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
    //this->m_pJvm->getMetaContents(sAbsPathName, &metaContents);
    if(!bIsBigFile)
    {
        emit getMetaContents(sAbsPathName, pMetaContents);          //needs connect(...Qt::BlockingQueuedConnection);
    }

    enrichMetaContents(sAbsPathName, pMetaContents, finfo);

    if(QThread::currentThread()->isInterruptionRequested())
    {
        return;
    }

    this->m_pLucyIndexer->index(sAbsPathName, pMetaContents, finfo);

    delete pMetaContents;
    pMetaContents = nullptr;

    m_iIndexedFiles++;
}

void indexerWorker::enrichMetaContents(QString sAbsPathName, QMap<QString, QStringList>* pMetas, QFileInfo finfo)
{
    this->addMetaContents(pMetas, QString::fromStdWString(FIELDNAME_ABSPATHNAME), sAbsPathName);

    {
        QString sDir(finfo.path());
        this->addMetaContents(pMetas, "dir", sDir);
    }
    {
        QString sFN(finfo.fileName());  //with ext
        this->addMetaContents(pMetas, QString::fromStdWString(FIELDNAME_FILENAME), sFN);
    }
    {
        QString sExt(finfo.suffix());   //or completeSuffix
        this->addMetaContents(pMetas, "extension", sExt);
    }
    {
        qint64 iSize = finfo.size();
        this->addMetaContents(pMetas, "size", QString::number(iSize));
    }
    {
        QDateTime lastMod = finfo.lastModified();
        uint uiLastMod = lastMod.toTime_t();
        this->addMetaContents(pMetas, "lastmodified", QString::number(uiLastMod));
    }
}

void indexerWorker::addMetaContents(QMap<QString, QStringList>* pMetas, QString sMetaName, QString sMetaValue)
{
    bool bContains = pMetas->contains(sMetaName);

    if(bContains)
    {
        pMetas->operator [](sMetaName) << sMetaValue;
    }
    else
    {
        QStringList sl;
        sl << sMetaValue;
        pMetas->operator [](sMetaName) = sl;
    }
}

int indexerWorker::getNrOfIndexedFiles()
{
    return this->m_iIndexedFiles;
}
int indexerWorker::getNrOfFilesInIndex()
{
    if(this->m_pLucyIndexer)
        return this->m_pLucyIndexer->getNrOfFilesInIndex();
    return 0;
}
int indexerWorker::getNrOfFoundFiles()
{
    return this->m_iFoundFiles;
}

int indexerWorker::getIndexTime()
{
    return this->m_iIndexingTime;
}

QString indexerWorker::getIndexedDir()
{
    return this->m_sDir2Index;
}

bool indexerWorker::openIndex()
{
    if(!m_pLucyIndexer)
    {
        m_pLucyIndexer = new lucyindexer(this->m_pLogger, this);
        m_pLucyIndexer->open(this->m_sDir2Index);
    }
    return true;
}
void indexerWorker::close(bool bDeleteCompleteIndex/*=false*/)
{
    if(m_pLucyIndexer)
    {
        m_pLucyIndexer->close(bDeleteCompleteIndex);
        delete m_pLucyIndexer;
    }
    m_pLucyIndexer = nullptr;
}

indexerWorker::~indexerWorker()
{
    close();
}
lucyindexer* indexerWorker::getIndexer()
{
    return this->m_pLucyIndexer;
}

void indexerWorker::finishIndexing(bool bInterruptionRequested)
{
    m_iIndexingTime = t.elapsed();
    m_pLogger->inf("an indexer thread finished. found#:"+QString::number(getNrOfFoundFiles())+" indexed#:"+QString::number(getNrOfIndexedFiles())+" NrOfFilesInIdx:"+QString::number(getNrOfFilesInIndex())+" time:"+logger::t_elapsed(getIndexTime())+"  dir:"+getIndexedDir());
    close();
    if(bInterruptionRequested)
    {
        QThread::currentThread()->quit();
        QThread::currentThread()->terminate();
    }
}

int indexerWorker::delDeletedFiles()
{
    m_pLogger->wrn("delDeletedFiles start "+this->m_sDir2Index);
    int iDeletedDocs = 0;
    //QMutexLocker ml(lucy::getIndexerLock());  //not needed

    IndexModifier* pIndexer = m_pLucyIndexer->getIndexer();
    int32_t maxDoc = pIndexer->docCount();
    for(int i=maxDoc-1; i>-1; i--)
    {
        if(QThread::currentThread()->isInterruptionRequested()) { break; }
        Document doc;
        if(!pIndexer->document(i, doc))
        {
            m_pLogger->wrn("!doc for nr:"+QString::number(i)+"/"+QString::number(maxDoc));
            continue;
        }
        {
            QString sAbsPathName = QString::fromStdWString(doc.get(FIELDNAME_ABSPATHNAME));
            if(str::isempty(sAbsPathName, true))
            {
                m_pLogger->wrn("!doc-absfn for nr:"+QString::number(i)+"/"+QString::number(maxDoc));
                continue;
            }

            bool bExists = QFile::exists(sAbsPathName);
            if( !bExists)
            {
                if(QThread::currentThread()->isInterruptionRequested()) { break; }
                pIndexer->deleteDocument(i);
                iDeletedDocs++;
            }
        }
    }

    m_pLogger->wrn("delDeletedFiles end (deleteds: "+QString::number(iDeletedDocs)+") "+this->m_sDir2Index);
    return iDeletedDocs;
}
void indexerWorker::fillIdxInfo(IdxInfo* idxi)
{
    if(!this->m_pLucyIndexer)
    {
        m_pLogger->wrn("no lucyIndexer to fill info");
        return;
    }
    this->m_pLucyIndexer->fillIdxInfo(idxi);
}
