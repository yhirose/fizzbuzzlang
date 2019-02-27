#include "peglib.h"

std::shared_ptr<peg::Ast> parse(const std::vector<char>& source, std::ostream& out) {
  peg::parser parser(R"(
    # Syntax Rules
    START                   <- _ EXPRESSION _
    EXPRESSION              <- TERNARY
    TERNARY                 <- CONDITION (_ '?' _ EXPRESSION _ ':' _ EXPRESSION)?
    CONDITION               <- MULTIPLICATIVE (_ ConditionOperator _ MULTIPLICATIVE)?
    MULTIPLICATIVE          <- CALL (_ MultiplicativeOperator _ CALL)*
    CALL                    <- PRIMARY (__ EXPRESSION)?
    PRIMARY                 <- FOR / Identifier / '(' _ EXPRESSION _ ')' / String / Number
    FOR                     <- 'for' __ Identifier __ 'from' __ Number __ 'to' __ Number __ EXPRESSION

    # Token Rules
    ConditionOperator       <- '=='
    MultiplicativeOperator  <- '%'
    Identifier              <- !Keyword [a-zA-Z][a-zA-Z0-9_]*
    String                  <- "'" < (!['] .)* > "'"
    Number                  <- [0-9]+
    ~_                      <- Whitespace*
    ~__                     <- Whitespace+
    Whitespace              <- [ \t\r\n]
    Keyword                 <- 'for' / 'from' / 'to'
  )");

  parser.enable_ast();

  parser.log = [&](size_t ln, size_t col, const std::string& msg) {
    out << ln << ":" << col << ": " << msg << std::endl;
  };

  std::shared_ptr<peg::Ast> ast;
  if (parser.parse_n(source.data(), source.size(), ast)) {
    return peg::AstOptimizer(true).optimize(ast);
  }

  return nullptr;
}
