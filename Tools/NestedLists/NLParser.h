#ifndef NL_PARSER_H
#define NL_PARSER_H

#include "NestedList.h"
#include <stack>
#include <vector>

#define YYSTYPE ListExpr

class NLScanner;

class NLParser
{
 public:
  NLParser( NestedList* nestedList, std::istream* ip = 0, std::ostream* op = 0 );
  virtual ~NLParser();
  int         yyparse();
  ListExpr    GetNestedList() { return listExpression; }
 protected:
  ListExpr    listExpression;
  stack<ListExpr> lists;
  int         yylex(); //inline int NLParser::yylex() { return (GetLexer()->yylex()); }
  void        yyerror( char* );
  std::istream*    isp;  // istream being parsed
  std::ostream*    osp;  // ostream being output to
  NLScanner*  lex;  // Lexical analyzer to use;
  NestedList* nl;

  static ListExpr    yylval;
  static int         yychar;
  static int         yynerrs;
  static int         yydebug;

  friend class NLScanner;
};

#endif

