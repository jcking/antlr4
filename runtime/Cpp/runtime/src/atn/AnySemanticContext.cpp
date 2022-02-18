#include "atn/AnySemanticContext.h"

using namespace antlr4::atn;

namespace {

struct GetVisitor final {
  const SemanticContext& operator()(const std::monostate &monostate) const {
    static_cast<void>(monostate);
    throw std::bad_variant_access();
  }

  template <typename T>
  const SemanticContext& operator()(const T &semanticContext) const {
    return semanticContext;
  }
};

}

const SemanticContext& AnySemanticContext::get() const {
  return std::visit(GetVisitor{}, _variant);
}
