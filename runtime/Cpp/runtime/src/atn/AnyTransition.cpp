#include "atn/AnyTransition.h"

using namespace antlr4::atn;

namespace {

struct GetVisitor final {
  template <typename T>
  Transition& operator()(T &transition) const {
    return transition;
  }
};

struct ConstGetVisitor final {
  template <typename T>
  const Transition& operator()(const T &transition) const {
    return transition;
  }
};

}

Transition& AnyTransition::get() {
  return std::visit(GetVisitor{}, _variant);
}

const Transition& AnyTransition::get() const {
  return std::visit(ConstGetVisitor{}, _variant);
}
