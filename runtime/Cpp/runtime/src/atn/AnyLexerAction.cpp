#include "atn/AnyLexerAction.h"

using namespace antlr4::atn;

namespace {

struct GetVisitor final {
  template <typename T>
  const LexerAction& operator()(const T &lexerAction) const {
    return lexerAction;
  }
};

struct GetSharedVisitor final {
  template <typename T>
  std::shared_ptr<const LexerAction> operator()(const T &lexerAction) const {
    return std::make_shared<T>(lexerAction);
  }
};

}

const LexerAction& AnyLexerAction::get() const {
  return std::visit(GetVisitor{}, _variant);
}

std::shared_ptr<const LexerAction> AnyLexerAction::getShared() const {
  return std::visit(GetSharedVisitor{}, _variant);
}
