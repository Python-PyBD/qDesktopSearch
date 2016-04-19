#ifndef LUCY_H
#define LUCY_H

#include <QObject>
#include "globalinclude.h"
#include "CLucene.h"
#include "CLucene/index/IndexModifier.h"
#include "CLucene/config/repl_wchar.h"
#include "CLucene/config/repl_tchar.h"
#include "CLucene/highlighter/Highlighter.h"
#include "CLucene/highlighter/QueryScorer.h"
#include "CLucene/highlighter/Highlighter.h"
#include "CLucene/highlighter/TokenGroup.h"
#include "CLucene/highlighter/SimpleHTMLFormatter.h"
#include "CLucene/highlighter/SimpleFragmenter.h"
#include "logger.h"
#include "cfg.h"
#include "str.h"
#include <QApplication>

using namespace lucene;
using namespace lucene::analysis;
using namespace lucene::analysis::standard;
using namespace lucene::store;
using namespace lucene::index;
using namespace lucene::search;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::util;
CL_NS_USE2(search, highlight);

class lucy : public QObject
{
    Q_OBJECT
public:
    explicit lucy(logger* pLogger, QObject *parent = 0);
    ~lucy();
    virtual void open(QString sDir2Index);
    virtual void close();
    lucene::analysis::Analyzer* getAnalyzer();
    Directory* getDirectory();
    void setDirectory(Directory* pDirectory);

protected:
    logger* m_pLogger;
    lucene::analysis::Analyzer* m_pAnalyzer;
    Directory* m_pDirectory;
    QString m_sDir2Index;
    QString m_sDirIndex;

signals:

public slots:
};

#endif // LUCY_H
