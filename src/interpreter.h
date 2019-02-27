#include "environment.h"

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
    auto call_env = std::make_shared<Environment>();
    call_env->set_value(ident, Value(i));
    call_env->append_outer(env);
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
