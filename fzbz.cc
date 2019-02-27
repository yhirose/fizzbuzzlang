#include <fstream>
#include "peglib.h"

using namespace std;

bool read_file(const char* path, vector<char>& buf) {
  ifstream ifs(path, ios::in);
  if (ifs.fail()) {
    return false;
  }
  buf.resize(static_cast<unsigned int>(ifs.seekg(0, ios::end).tellg()));
  ifs.seekg(0, ios::beg).read(&buf[0], static_cast<streamsize>(buf.size()));
  return true;
}

std::shared_ptr<peg::Ast> parse(const vector<char>& source) {
  static peg::parser parser(R"(
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
  parser.log = [&](size_t ln, size_t col, const string& msg) {
    cerr << ln << ":" << col << ": " << msg << endl;
  };

  std::shared_ptr<peg::Ast> ast;
  if (parser.parse_n(source.data(), source.size(), ast)) {
    return peg::AstOptimizer(true).optimize(ast);
  }
  return nullptr;
}

class Interpreter {
 public:
  struct Value {
    enum Type { Nil, Bool, Long, String };

    Value() : type(Nil) {}
    explicit Value(bool b) : type(Bool), v(b) {}
    explicit Value(long l) : type(Long), v(l) {}
    explicit Value(const std::string& s) : type(String), v(s) {}

    bool to_bool() const {
      switch (type) {
        case Bool:
          return v.get<bool>();
        case Long:
          return v.get<long>() != 0;
        default:
          throw std::runtime_error("type error.");
      }
    }

    long to_long() const {
      switch (type) {
        case Long:
          return v.get<long>();
        default:
          throw std::runtime_error("type error.");
      }
    }

    std::string to_string() const {
      switch (type) {
        case String:
          return v.get<std::string>();
        default:
          throw std::runtime_error("type error.");
      }
    }

    std::string str() const {
      switch (type) {
        case Nil:
          return "nil";
        case Bool:
          return to_bool() ? "true" : "false";
        case Long:
          return std::to_string(to_long());
        case String:
          return to_string();
        default:
          throw std::logic_error("invalid internal condition.");
      }
    }

    bool operator==(const Value& rhs) const {
      switch (type) {
        case Nil:
          return rhs.type == Nil;
        case Bool:
          return to_bool() == rhs.to_bool();
        case Long:
          return to_long() == rhs.to_long();
        case String:
          return to_string() == rhs.to_string();
        default:
          throw std::logic_error("invalid internal condition.");
      }
    }

    Type type;
    peg::any v;
  };

  static Value run(const peg::Ast& ast) {
    auto env = make_shared<Environment>();
    env->set_function("puts", [](const Value& val) {
      cout << val.str() << endl;
      return Value();
    });

    try {
      return eval(ast, env);
    } catch (const exception& e) {
      cerr << e.what() << endl;
      return Value();
    }
  }

 private:
  typedef function<Value(const Value&)> Function;

  struct Environment {
    void append_outer(std::shared_ptr<Environment> outer) {
      if (this->outer) {
        this->outer->append_outer(outer);
      } else {
        this->outer = outer;
      }
    }

    bool has(const std::string& s) const {
      if (values.find(s) != values.end()) {
        return true;
      }
      return outer && outer->has(s);
    }

    const Value& get_value(const std::string& s) const {
      if (values.find(s) != values.end()) {
        return values.at(s);
      } else if (outer) {
        return outer->get_value(s);
      }
      throw std::runtime_error("undefined variable '" + s + "'...");
    }

    Function get_function(const std::string& s) const {
      if (functions.find(s) != functions.end()) {
        return functions.at(s);
      } else if (outer) {
        return outer->get_function(s);
      }
      throw std::runtime_error("undefined variable '" + s + "'...");
    }

    void set_value(const std::string& s, const Value& val) { values[s] = val; }

    void set_function(const std::string& s, Function fn) { functions[s] = fn; }

    std::shared_ptr<Environment> outer;
    std::map<std::string, Value> values;
    std::map<std::string, Function> functions;
  };

  static Value eval(const peg::Ast& ast, std::shared_ptr<Environment> env) {
    using namespace peg::udl;

    switch (ast.tag) {
      case "TERNARY"_:
        return eval_ternary(ast, env);
      case "CONDITION"_:
        return eval_condition(ast, env);
      case "MULTIPLICATIVE"_:
        return eval_multiplicative(ast, env);
      case "CALL"_:
        return eval_call(ast, env);
      case "FOR"_:
        return eval_for(ast, env);
      case "Identifier"_:
        return eval_identifier(ast, env);
      case "String"_:
        return eval_string(ast, env);
      case "Number"_:
        return eval_number(ast, env);
    }

    return Value();
  }

  static Value eval_ternary(const peg::Ast& ast,
                            std::shared_ptr<Environment> env) {
    auto cond = eval(*ast.nodes[0], env).to_bool();
    auto val1 = eval(*ast.nodes[1], env);
    auto val2 = eval(*ast.nodes[2], env);
    return cond ? val1 : val2;
  }

  static Value eval_condition(const peg::Ast& ast,
                              std::shared_ptr<Environment> env) {
    auto lhs = eval(*ast.nodes[0], env).to_long();
    auto rhs = eval(*ast.nodes[2], env).to_long();
    auto ret = lhs == rhs;
    return Value(ret);
  }

  static Value eval_multiplicative(const peg::Ast& ast,
                                   std::shared_ptr<Environment> env) {
    auto l = eval(*ast.nodes[0], env).to_long();
    for (auto i = 1; i < ast.nodes.size(); i += 2) {
      auto r = eval(*ast.nodes[i + 1], env).to_long();
      l = l % r;
    }
    return Value(l);
  }

  static Value eval_call(const peg::Ast& ast,
                         std::shared_ptr<Environment> env) {
    auto fn = env->get_function(ast.nodes[0]->token);
    auto val = eval(*ast.nodes[1], env);
    return fn(val);
  }

  static Value eval_for(const peg::Ast& ast, std::shared_ptr<Environment> env) {
    auto ident = ast.nodes[0]->token;
    auto from = eval(*ast.nodes[1], env).to_long();
    auto to = eval(*ast.nodes[2], env).to_long();
    auto& expr = *ast.nodes[3];

    for (auto i = from; i <= to; i++) {
      auto call_env = make_shared<Environment>();
      call_env->set_value(ident, Value(i));
      call_env->append_outer(env);
      eval(expr, call_env);
    }

    return Value();
  }

  static Value eval_identifier(const peg::Ast& ast,
                               std::shared_ptr<Environment> env) {
    return Value(env->get_value(ast.token));
  }

  static Value eval_string(const peg::Ast& ast,
                           std::shared_ptr<Environment> env) {
    return Value(ast.token);
  }

  static Value eval_number(const peg::Ast& ast,
                           std::shared_ptr<Environment> env) {
    return Value(stol(ast.token));
  }
};

int main(int argc, const char** argv) {
  if (argc < 2) {
    cerr << "usage: fzbz [source file path]" << endl;
    return 1;
  }
  auto path = argv[1];

  vector<char> source;
  if (!read_file(path, source)) {
    cerr << "can't open the source file." << endl;
    return -1;
  }

  auto ast = parse(source);
  if (!ast) {
    return -2;
  }

  Interpreter::run(*ast);
  return 0;
}
