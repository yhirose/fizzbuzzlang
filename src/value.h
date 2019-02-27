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
        return v.get<bool>();
      case Type::Long:
        return v.get<long>() != 0;
      default:
        throw std::runtime_error("type error.");
    }
  }

  long to_long() const {
    switch (type) {
      case Type::Long:
        return v.get<long>();
      default:
        throw std::runtime_error("type error.");
    }
  }

  std::string to_string() const {
    switch (type) {
      case Type::String:
        return v.get<std::string>();
      default:
        throw std::runtime_error("type error.");
    }
  }

  Function to_function() const {
    switch (type) {
      case Type::Function:
        return v.get<Function>();
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
};
