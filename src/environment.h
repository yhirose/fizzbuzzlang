#include "value.h"

struct Environment {
  std::shared_ptr<Environment> outer;
  std::map<std::string, Value> values;

  void append_outer(std::shared_ptr<Environment> outer) {
    if (this->outer) {
      this->outer->append_outer(outer);
    } else {
      this->outer = outer;
    }
  }

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
