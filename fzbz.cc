//
//  FizzBuzzLang
//  A Programming Language just for writing Fizz Buzz program. :)
//
//  Copyright (c) 2020 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <fstream>

#include "peglib.h"

using namespace std;
using namespace peg;
using namespace peg::udl;

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

shared_ptr<Ast> parse(const vector<char>& source, ostream& out) {
  parser parser(R"(
    # Syntax Rules
    EXPRESSION              <- TERNARY
    TERNARY                 <- CONDITION ('?' EXPRESSION ':' EXPRESSION)?
    CONDITION               <- MULTIPLICATIVE (ConditionOperator MULTIPLICATIVE)?
    MULTIPLICATIVE          <- CALL (MultiplicativeOperator CALL)*
    CALL                    <- PRIMARY (EXPRESSION)?
    PRIMARY                 <- FOR / Identifier / '(' EXPRESSION ')' / String / Number
    FOR                     <- 'for' Identifier 'from' Number 'to' Number EXPRESSION

    # Token Rules
    ConditionOperator       <- '=='
    MultiplicativeOperator  <- '%'
    Identifier              <- !Keyword < [a-zA-Z][a-zA-Z0-9_]* >
    String                  <- "'" < ([^'] .)* > "'"
    Number                  <- < [0-9]+ >

    Keyword                 <- ('for' / 'from' / 'to') ![a-zA-Z]
    %whitespace             <- [ \t\r\n]*
  )");

  parser.enable_ast();

  parser.log = [&](size_t ln, size_t col, const string& msg) {
    out << ln << ":" << col << ": " << msg << endl;
  };

  shared_ptr<Ast> ast;
  if (parser.parse_n(source.data(), source.size(), ast)) {
    return AstOptimizer(true).optimize(ast);
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
// Value
//-----------------------------------------------------------------------------

struct Value;
using Function = function<Value(const Value&)>;

struct Value {
  enum class Type { Nil, Bool, Long, String, Function };
  Type type;
  any v;

  // Constructor
  Value() : type(Type::Nil) {}
  explicit Value(bool b) : type(Type::Bool), v(b) {}
  explicit Value(long l) : type(Type::Long), v(l) {}
  explicit Value(string_view s) : type(Type::String), v(s) {}
  explicit Value(Function f) : type(Type::Function), v(std::move(f)) {}

  // Cast value
  bool to_bool() const {
    switch (type) {
      case Type::Bool:
        return any_cast<bool>(v);
      case Type::Long:
        return any_cast<long>(v) != 0;
      default:
        throw runtime_error("type error.");
    }
  }

  long to_long() const {
    switch (type) {
      case Type::Long:
        return any_cast<long>(v);
      default:
        throw runtime_error("type error.");
    }
  }

  string_view to_string_view() const {
    switch (type) {
      case Type::String:
        return any_cast<string_view>(v);
      default:
        throw runtime_error("type error.");
    }
  }

  Function to_function() const {
    switch (type) {
      case Type::Function:
        return any_cast<Function>(v);
      default:
        throw runtime_error("type error.");
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
        return to_string_view() == rhs.to_string_view();
      default:
        throw logic_error("invalid internal condition.");
    }
  }

  // String representation
  string str() const {
    switch (type) {
      case Type::Nil:
        return "nil";
      case Type::Bool:
        return to_bool() ? "true" : "false";
      case Type::Long:
        return std::to_string(to_long());
      case Type::String:
        return string(to_string_view());
      default:
        throw logic_error("invalid internal condition.");
    }
  }
};

//-----------------------------------------------------------------------------
// Environment
//-----------------------------------------------------------------------------

struct Environment {
  shared_ptr<Environment> outer;
  map<string_view, Value> values;

  Environment(shared_ptr<Environment> outer = nullptr) : outer(outer) {}

  const Value& get_value(string_view s) const {
    if (values.find(s) != values.end()) {
      return values.at(s);
    } else if (outer) {
      return outer->get_value(s);
    }
    throw runtime_error("undefined variable '" + string(s) + "'...");
  }

  void set_value(string_view s, const Value& val) { values[s] = val; }
};

//-----------------------------------------------------------------------------
// Interpreter
//-----------------------------------------------------------------------------

Value eval(const Ast& ast, shared_ptr<Environment> env);

Value eval_ternary(const Ast& ast, shared_ptr<Environment> env) {
  auto cond = eval(*ast.nodes[0], env).to_bool();
  auto val1 = eval(*ast.nodes[1], env);
  auto val2 = eval(*ast.nodes[2], env);
  return cond ? val1 : val2;
}

Value eval_condition(const Ast& ast, shared_ptr<Environment> env) {
  auto lhs = eval(*ast.nodes[0], env);
  auto rhs = eval(*ast.nodes[2], env);
  auto ret = lhs == rhs;
  return Value(ret);
}

Value eval_multiplicative(const Ast& ast, shared_ptr<Environment> env) {
  auto l = eval(*ast.nodes[0], env).to_long();
  for (auto i = 1; i < ast.nodes.size(); i += 2) {
    auto r = eval(*ast.nodes[i + 1], env).to_long();
    l = l % r;
  }
  return Value(l);
}

Value eval_call(const Ast& ast, shared_ptr<Environment> env) {
  auto fn = env->get_value(ast.nodes[0]->token).to_function();
  auto val = eval(*ast.nodes[1], env);
  return fn(val);
}

Value eval_for(const Ast& ast, shared_ptr<Environment> env) {
  auto ident = ast.nodes[0]->token;
  auto from = eval(*ast.nodes[1], env).to_long();
  auto to = eval(*ast.nodes[2], env).to_long();
  auto& expr = *ast.nodes[3];

  for (auto i = from; i <= to; i++) {
    auto call_env = make_shared<Environment>(env);
    call_env->set_value(ident, Value(i));
    eval(expr, call_env);
  }

  return Value();
}

Value eval(const Ast& ast, shared_ptr<Environment> env) {
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
      return Value(ast.token_to_number<long>());
  }

  return Value();
}

Value interpret(const Ast& ast) {
  auto env = make_shared<Environment>();

  env->set_value("puts", Value(Function([](auto& val) {
                   cout << val.str() << endl;
                   return Value();
                 })));

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
