#include "searcher.h"
#include "lucysearcher.h"
#include "seacherHighlightFormatter.h"

searcher::searcher(QObject *parent) : QObject(parent),
    m_pIndexer(nullptr),
    m_pLog(nullptr),
    m_pMultiSearcher(nullptr),
    m_aquerys(nullptr),
    m_searchables(nullptr),
    m_query(nullptr),
    m_hits(nullptr)
{
    cleanup(true);
}

int searcher::search(QList<QPair<QString, QString>> lpSearchinputs)
{
    cleanup(false);

    if(lpSearchinputs.size() < 1)
    {
        this->m_pLog->wrn("nothing to search for!");
        return 0;
    }

    QMapIterator<QString, indexerThread*> i(*this->m_pIndexer->getIndexers());
    m_searchables = _CL_NEWARRAY(Searchable*,this->m_pIndexer->getIndexers()->size()+1);
    int iIdx = 0;
    while (i.hasNext())
    {
        i.next();

        lucysearcher* pLucySearcher = new lucysearcher(this->m_pLog);

        //indexerThread* indexerThread = i.value();
        //pLucySearcher->open(indexerThread->getWorker()->getIndexer()->getDirectory());
        QString sDir2Index(i.key());
        pLucySearcher->open(sDir2Index);    //crash at the second search

        m_searchables[iIdx] = pLucySearcher->getSearcher();
        m_lucysearchables.append(pLucySearcher);
        iIdx++;
    }
    m_searchables[this->m_pIndexer->getIndexers()->size()] = nullptr;

    m_pMultiSearcher = new MultiSearcher(m_searchables);

    m_aquerys   = _CLNEW CL_NS(util)::ObjectArray<Query>(lpSearchinputs.length());
    m_query     = _CLNEW BooleanQuery();

    for(int i=0;i<lpSearchinputs.length();i++)
    {
        QPair<QString, QString> pSearchInput = lpSearchinputs.at(i);
        QString sSearchString = pSearchInput.first;
        QString sSearchField  = pSearchInput.second;
        m_query->add(QueryParser::parse(sSearchString.toStdWString().c_str(), sSearchField.toStdWString().c_str(), lucy::getNewAnalyzer()), true/*handle delete*/, BooleanClause::SHOULD);
    }

    //TODO: sort by score
    m_hits = m_pMultiSearcher->search(m_query);

    int iHits = (int)(m_hits == nullptr ? 0 : m_hits->length());

    this->m_pLog->inf("hits:"+QString::number(iHits)+" query:"+QString::fromStdWString(m_query->toString(_T("text"))));

    return iHits;
}

int searcher::getHitCount()
{
    int iHits = (int)(m_hits == nullptr ? 0 : m_hits->length());
    return iHits;
}

QString searcher::GetHitAttr(int iHitNr, QString sAttrName)
{
    Document* d = &(m_hits->doc(iHitNr));
    const TCHAR* pAttrValue = d->get(sAttrName.toStdWString().c_str());

    if(!pAttrValue)
    {
        this->m_pLog->wrn("no "+sAttrName+" hitNr:"+QString::number(iHitNr)+"");
        return "";
    }
    return QString::fromStdWString(pAttrValue);
}

QString searcher::GetHitEnv(int iHitNr)
{
    QueryScorer scorer(m_query);
    seacherHighlightFormatter hl_formatter;
    seacherHighlightFormatter* pHLFormatter = &hl_formatter;
    Formatter* pFormatter = pHLFormatter;
    Highlighter highlighter(pFormatter, &scorer);
    SimpleFragmenter frag(50);
    highlighter.setTextFragmenter(&frag);

    {
        const TCHAR* text = m_hits->doc(iHitNr).get(FIELDNAME_FULLTEXT);
        if(!text)return "";
        int maxNumFragmentsRequired     = 3;
        const TCHAR* fragmentSeparator  = _T("...");
        StringReader reader(text);
        TokenStream* tokenStream = this->m_lucysearchables.at(0)->getAnalyzer()->tokenStream(FIELDNAME_FULLTEXT, &reader);
        if(!tokenStream)return "";

        TCHAR* result =
                highlighter.getBestFragments(
                tokenStream,
                text,
                maxNumFragmentsRequired,
                fragmentSeparator);

        if(!result)return "";

        QString sHitEnv = QString::fromStdWString(result);

        _CLDELETE_CARRAY(result);
        _CLDELETE(tokenStream);

        sHitEnv.replace('\n', ' ').replace('\t', ' ').replace("  ", " ");

        return sHitEnv;
    }
}

void searcher::setEnv(logger* pLog, indexer* pIndexer)
{
    this->m_pLog    = pLog;
    this->m_pIndexer= pIndexer;
}
void searcher::cleanup(bool bConstructor)
{
    Q_UNUSED(bConstructor)
    if(m_aquerys)
    {
        _CLLDELETE(m_aquerys);
        m_aquerys = nullptr;
    }
    if(m_query)
    {
        delete m_query;
        m_query = nullptr;
    }
    if(m_hits)
    {
        delete m_hits;
        m_hits = nullptr;
    }
    if(m_pMultiSearcher)
    {
        m_pMultiSearcher->close();
        delete m_pMultiSearcher;
        m_pMultiSearcher = nullptr;
    }

    if(m_searchables)_CLDELETE_ARRAY(m_searchables);
    if(this->m_aquerys)
    {
        this->m_aquerys->deleteValues();
        _CLLDELETE(this->m_aquerys);
        m_aquerys = nullptr;
    }

    for(int i=0;i<m_lucysearchables.length();i++)
    {
        m_lucysearchables.at(i)->setDirectory(nullptr);
        m_lucysearchables.at(i)->setSearcher(nullptr);
        delete m_lucysearchables.at(i);
    }
    m_lucysearchables.clear();
}
