#include "peglib.h"
#include <fstream>
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

//-----------------------------------------------------------------------------
// Parser
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Value
//-----------------------------------------------------------------------------

struct Value;

typedef std::function<Value(const Value&)> Function;

struct Value {
  enum class Type { Nil, Bool, Long, String, Function };
  Type type;
  peg::any v;

  // Constructor
  Value() : type(Type::Nil) {}
  explicit Value(bool b) : type(Type::Bool), v(b) {}
  explicit Value(long l) : type(Type::Long), v(l) {}
  explicit Value(const std::string& s) : type(Type::String), v(s) {}
  explicit Value(const Function& f) : type(Type::Function), v(f) {}

  // Cast value
  bool to_bool() const {
    switch (type) {
      case Type::Bool:
        return get<bool>();
      case Type::Long:
        return get<long>() != 0;
      default:
        throw std::runtime_error("type error.");
    }
  }

  long to_long() const {
    switch (type) {
      case Type::Long:
        return get<long>();
      default:
        throw std::runtime_error("type error.");
    }
  }

  std::string to_string() const {
    switch (type) {
      case Type::String:
        return get<std::string>();
      default:
        throw std::runtime_error("type error.");
    }
  }

  Function to_function() const {
    switch (type) {
      case Type::Function:
        return get<Function>();
      default:
        throw std::runtime_error("type error.");
    }
  }

  // Comparison
  bool operator==(const Value& rhs) const {
    switch (type) {
      case Type::Nil:
        return rhs.type == Type::Nil;
      case Type::Bool:
        return to_bool() == rhs.to_bool();
      case Type::Long:
        return to_long() == rhs.to_long();
      case Type::String:
        return to_string() == rhs.to_string();
      default:
        throw std::logic_error("invalid internal condition.");
    }
  }

  // String representation
  std::string str() const {
    switch (type) {
      case Type::Nil:
        return "nil";
      case Type::Bool:
        return to_bool() ? "true" : "false";
      case Type::Long:
        return std::to_string(to_long());
      case Type::String:
        return to_string();
      default:
        throw std::logic_error("invalid internal condition.");
    }
  }

  // Cast `any` value
  template <typename T> const T &get() const {
    return peg::any_cast<T>(v);
  }
};

//-----------------------------------------------------------------------------
// Environment
//-----------------------------------------------------------------------------

struct Environment {
  std::shared_ptr<Environment> outer;
  std::map<std::string, Value> values;

  Environment(std::shared_ptr<Environment> outer = nullptr): outer(outer) {}

  const Value& get_value(const std::string& s) const {
    if (values.find(s) != values.end()) {
      return values.at(s);
    } else if (outer) {
      return outer->get_value(s);
    }
    throw std::runtime_error("undefined variable '" + s + "'...");
  }

  void set_value(const std::string& s, const Value& val) { values[s] = val; }
};

//-----------------------------------------------------------------------------
// Interpreter
//-----------------------------------------------------------------------------

Value eval(const peg::Ast& ast, std::shared_ptr<Environment> env);

Value eval_ternary(const peg::Ast& ast, std::shared_ptr<Environment> env) {
  auto cond = eval(*ast.nodes[0], env).to_bool();
  auto val1 = eval(*ast.nodes[1], env);
  auto val2 = eval(*ast.nodes[2], env);
  return cond ? val1 : val2;
}

Value eval_condition(const peg::Ast& ast, std::shared_ptr<Environment> env) {
  auto lhs = eval(*ast.nodes[0], env);
  auto rhs = eval(*ast.nodes[2], env);
  auto ret = lhs == rhs;
  return Value(ret);
}

Value eval_multiplicative(const peg::Ast& ast,
                          std::shared_ptr<Environment> env) {
  auto l = eval(*ast.nodes[0], env).to_long();
  for (auto i = 1; i < ast.nodes.size(); i += 2) {
    auto r = eval(*ast.nodes[i + 1], env).to_long();
    l = l % r;
  }
  return Value(l);
}

Value eval_call(const peg::Ast& ast, std::shared_ptr<Environment> env) {
  auto fn = env->get_value(ast.nodes[0]->token).to_function();
  auto val = eval(*ast.nodes[1], env);
  return fn(val);
}

Value eval_for(const peg::Ast& ast, std::shared_ptr<Environment> env) {
  auto ident = ast.nodes[0]->token;
  auto from = eval(*ast.nodes[1], env).to_long();
  auto to = eval(*ast.nodes[2], env).to_long();
  auto& expr = *ast.nodes[3];

  for (auto i = from; i <= to; i++) {
    auto call_env = std::make_shared<Environment>(env);
    call_env->set_value(ident, Value(i));
    eval(expr, call_env);
  }

  return Value();
}

Value eval(const peg::Ast& ast, std::shared_ptr<Environment> env) {
  using namespace peg::udl;

  switch (ast.tag) {
    // Rules
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

    // Tokens
    case "Identifier"_:
      return Value(env->get_value(ast.token));
    case "String"_:
      return Value(ast.token);
    case "Number"_:
      return Value(stol(ast.token));
  }

  return Value();
}

Function putsFn = [](const Value& val) -> Value {
  std::cout << val.str() << std::endl;
  return Value();
};

Value interpret(const peg::Ast& ast) {
  auto env = std::make_shared<Environment>();
  env->set_value("puts", Value(putsFn));
  return eval(ast, env);
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------

int main(int argc, const char** argv) {
  if (argc < 2) {
    cerr << "usage: fzbz [source file path]" << endl;
    return 1;
  }

  auto path = argv[1];

  vector<char> source;
  if (!read_file(path, source)) {
    cerr << "can't open the source file." << endl;
    return 2;
  }

  auto ast = parse(source, cerr);
  if (!ast) {
    return 3;
  }

  try {
    interpret(*ast);
  } catch (const exception& e) {
    cerr << e.what() << endl;
    return 4;
  }

  return 0;
}
