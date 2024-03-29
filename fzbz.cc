//
//  FizzBuzzLang
//  A Programming Language just for writing Fizz Buzz program. :)
//
//  Copyright (c) 2021 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <fstream>
#include <variant>

#include "peglib.h"

using namespace std;
using namespace peg;
using namespace peg::udl;

//-----------------------------------------------------------------------------
// Parser
//-----------------------------------------------------------------------------

shared_ptr<Ast> parse(const string& source, ostream& out) {
  parser parser(R"(
    # Syntax Rules
    EXPRESSION              ← TERNARY
    TERNARY                 ← CONDITION ('?' EXPRESSION ':' EXPRESSION)?
    CONDITION               ← MULTIPLICATIVE (ConditionOperator MULTIPLICATIVE)?
    MULTIPLICATIVE          ← CALL (MultiplicativeOperator CALL)*
    CALL                    ← PRIMARY (EXPRESSION)?
    PRIMARY                 ← FOR / Identifier / '(' EXPRESSION ')' / String / Number
    FOR                     ← 'for' Identifier 'from' Number 'to' Number EXPRESSION

    # Token Rules
    ConditionOperator       ← '=='
    MultiplicativeOperator  ← '%'
    Identifier              ← !Keyword < [a-zA-Z][a-zA-Z0-9_]* >
    String                  ← "'" < ([^'] .)* > "'"
    Number                  ← < [0-9]+ >

    Keyword                 ← ('for' / 'from' / 'to') ![a-zA-Z]
    %whitespace             ← [ \t\r\n]*
  )");

  parser.enable_ast();

  parser.set_logger([&](size_t ln, size_t col, const auto& msg) {
    out << ln << ":" << col << ": " << msg << endl;
  });

  shared_ptr<Ast> ast;
  if (parser.parse_n(source.data(), source.size(), ast)) {
    return parser.optimize_ast(ast);
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
// Value
//-----------------------------------------------------------------------------

struct Value;
using Function = function<Value(const Value&)>;

struct Value {
  variant<nullptr_t, bool, long, string_view, Function> v;

  Value() : v(nullptr) {}
  explicit Value(bool b) : v(b) {}
  explicit Value(long l) : v(l) {}
  explicit Value(string_view s) : v(s) {}
  explicit Value(Function f) : v(move(f)) {}

  template <typename T>
  T get() const {
    try {
      return std::get<T>(v);
    } catch (bad_variant_access&) {
      throw runtime_error("type error.");
    }
  }

  bool operator==(const Value& rhs) const {
    switch (v.index()) {
      case 0:
        return get<nullptr_t>() == rhs.get<nullptr_t>();
      case 1:
        return get<bool>() == rhs.get<bool>();
      case 2:
        return get<long>() == rhs.get<long>();
      case 3:
        return get<string_view>() == rhs.get<string_view>();
    }
  }

  string str() const {
    switch (v.index()) {
      case 0:
        return "nil";
      case 1:
        return get<bool>() ? "true" : "false";
      case 2:
        return to_string(get<long>());
      case 3:
        return string(get<string_view>());
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
    if (auto it = values.find(s); it != values.end()) {
      return it->second;
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
  auto cond = eval(*ast.nodes[0], env).get<bool>();
  auto val1 = eval(*ast.nodes[1], env);
  auto val2 = eval(*ast.nodes[2], env);
  return cond ? val1 : val2;
}

Value eval_condition(const Ast& ast, shared_ptr<Environment> env) {
  auto lhs = eval(*ast.nodes[0], env);
  auto rhs = eval(*ast.nodes[2], env);
  return Value(lhs == rhs);
}

Value eval_multiplicative(const Ast& ast, shared_ptr<Environment> env) {
  auto l = eval(*ast.nodes[0], env).get<long>();
  for (size_t i = 1; i < ast.nodes.size(); i += 2) {
    auto r = eval(*ast.nodes[i + 1], env).get<long>();
    l = l % r;
  }
  return Value(l);
}

Value eval_call(const Ast& ast, shared_ptr<Environment> env) {
  auto fn = env->get_value(ast.nodes[0]->token).get<Function>();
  auto val = eval(*ast.nodes[1], env);
  return fn(val);
}

Value eval_for(const Ast& ast, shared_ptr<Environment> env) {
  auto ident = ast.nodes[0]->token;
  auto from = eval(*ast.nodes[1], env).get<long>();
  auto to = eval(*ast.nodes[2], env).get<long>();
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
      return Value(env->get_value(ast.token));
    case "String"_:
      return Value(ast.token);
    case "Number"_:
      return Value(ast.token_to_number<long>());
    default:
      return Value();
  }
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

  ifstream file{argv[1]};
  if (!file) {
    cerr << "can't open the source file." << endl;
    return 2;
  }

  auto source =
      string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

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
