#include "atn/AnyPredictionContext.h"

using namespace antlr4::atn;

namespace {

struct GetVisitor final {
  const PredictionContext& operator()(const std::monostate &monostate) const {
    static_cast<void>(monostate);
    throw std::bad_variant_access();
  }

  template <typename T>
  const PredictionContext& operator()(const T &predictionContext) const {
    return predictionContext;
  }
};

}

const PredictionContext& AnyPredictionContext::get() const {
  return std::visit(GetVisitor{}, _variant);
}
