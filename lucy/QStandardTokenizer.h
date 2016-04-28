#ifndef QSTANDARDTOKENIZER
#define QSTANDARDTOKENIZER

#include "CLucene/analysis/AnalysisHeader.h" //required for Tokenizer
#include "CLucene/analysis/standard/StandardTokenizerConstants.h"
#include "CLucene/util/_FastCharStream.h"

#include "lucy.h"

using namespace lucene::analysis;
using namespace lucene::analysis::standard;

/** A grammar-based tokenizer constructed with JavaCC.
 *
 * <p> This should be a good tokenizer for most European-language documents:
 *
 * <ul>
 *   <li>Splits words at punctuation characters, removing punctuation. However, a 
 *     dot that's not followed by whitespace is considered part of a token.
 *   <li>Splits words at hyphens, unless there's a number in the token, in which case
 *     the whole token is interpreted as a product number and is not split.
 *   <li>Recognizes email addresses and internet hostnames as one token.
 * </ul>
 *
 * <p>Many applications have specific tokenizer needs.  If this tokenizer does
 * not suit your application, please consider copying this source code
 * directory to your project and maintaining your own grammar-based tokenizer.
 */
  class QStandardTokenizer: public Tokenizer
  {
  private:
    int32_t rdPos;
    int32_t tokenStart;

    // Advance by one character, incrementing rdPos and returning the character.
    int readChar();
    // Retreat by one character, decrementing rdPos.
    void unReadChar();

    // createToken centralizes token creation for auditing purposes.
	//Token* createToken(CL_NS(util)::StringBuffer* sb, TokenTypes tokenCode);
    inline Token* setToken(lucene::analysis::Token* t, CL_NS(util)::StringBuffer* sb, TokenTypes tokenCode);

    Token* ReadDotted(CL_NS(util)::StringBuffer* str, TokenTypes forcedType,Token* t);

	CL_NS(util)::BufferedReader* reader;
	bool deleteReader;
	CL_NS(util)::FastCharStream* rd;
  public:

    // Constructs a tokenizer for this Reader.
    QStandardTokenizer(CL_NS(util)::BufferedReader* reader, bool deleteReader=false);

    virtual ~QStandardTokenizer();

    /** Returns the next token in the stream, or false at end-of-stream.
    * The returned token's type is set to an element of
    * StandardTokenizerConstants::tokenImage. */
    Token* next(Token* token);

    // Reads for number like "1"/"1234.567", or IP address like "192.168.1.2".
    Token* ReadNumber(const TCHAR* previousNumber, const TCHAR prev, Token* t);

    Token* ReadAlphaNum(const TCHAR prev, Token* t);

    // Reads for apostrophe-containing word.
    Token* ReadApostrophe(CL_NS(util)::StringBuffer* str, Token* t);

    // Reads for something@... it may be a COMPANY name or a EMAIL address
    Token* ReadAt(CL_NS(util)::StringBuffer* str, Token* t);

    // Reads for COMPANY name like AT&T.
    Token* ReadCompany(CL_NS(util)::StringBuffer* str, Token* t);
    
    // Reads CJK characters
    Token* ReadCJK(const TCHAR prev, Token* t);

    virtual void reset(CL_NS(util)::Reader* _input);
};

#endif	//QSTANDARDTOKENIZER
