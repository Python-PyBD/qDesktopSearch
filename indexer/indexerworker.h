#ifndef INDEXERWORKER_H
#define INDEXERWORKER_H

#include <QObject>
#include <QTime>
#include <QDebug>
#include <QDirIterator>
#include "jvm.h"
#include "lucyindexer.h"
#include "logger.h"

class indexerWorker : public QObject
{
    Q_OBJECT
public:
    explicit indexerWorker(QString sDir2Index, logger* pLogger, jvm* pJvm, /*lucyindexer* pLucyIndexer, */QObject *parent = 0);
    ~indexerWorker();
    void close();
    int getNrOfIndexedFiles();
    int getNrOfFilesInIndex();
    int getNrOfFoundFiles();
    QString getIndexedDir();
    int getIndexTime();
    bool openIndex();
    lucyindexer* getIndexer();

protected:
    QString m_sDir2Index;
    logger* m_pLogger;
    lucyindexer* m_pLucyIndexer;
    jvm* m_pJvm;
    int m_iFoundFiles;
    int m_iIndexedFiles;
    int m_iIndexingTime;

    void dir(QString sDir);
    void file(QString sAbsPathName, QFileInfo finfo);
    void addMetaContents(QMap<QString, QStringList>* pMetas, QString sMetaName, QString sMetaValue);
    void enrichMetaContents(QString sAbsPathName, QMap<QString, QStringList>* pMetaContents, QFileInfo finfo);

signals:
    void finished();
    void getMetaContents(QString sAbsPathName, QMap<QString, QStringList>* pMetas);


public slots:
    void doWork();

};

#endif // INDEXERWORKER_H
