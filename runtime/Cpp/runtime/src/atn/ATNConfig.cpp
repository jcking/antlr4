/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "misc/MurmurHash.h"
#include "atn/PredictionContext.h"
#include "SemanticContext.h"
#include "atn/DecisionState.h"

#include "atn/ATNConfig.h"

using namespace antlr4::atn;

namespace {

/**
 * This field stores the bit mask for implementing the
 * {@link #isPrecedenceFilterSuppressed} property as a bit within the
 * existing {@link #reachesIntoOuterContext} field.
 */
inline constexpr size_t SUPPRESS_PRECEDENCE_FILTER = 0x40000000;

}

ATNConfig::ATNConfig(ATNState *state, size_t alt, AnyPredictionContext context)
    : ATNConfig(state, alt, std::move(context), 0, SemanticContext::none(), LexerActionExecutor()) {}

ATNConfig::ATNConfig(ATNState *state, size_t alt, AnyPredictionContext context, AnySemanticContext semanticContext)
    : ATNConfig(state, alt, std::move(context), 0, std::move(semanticContext), LexerActionExecutor()) {}

ATNConfig::ATNConfig(ATNState *state, size_t alt, AnyPredictionContext context, LexerActionExecutor lexerActionExecutor)
    : ATNConfig(state, alt, std::move(context), 0, SemanticContext::none(), std::move(lexerActionExecutor)) {}

ATNConfig::ATNConfig(ATNConfig const& other, AnySemanticContext semanticContext)
    : ATNConfig(other.state, other.alt, other.context, other.reachesIntoOuterContext, std::move(semanticContext), other.lexerActionExecutor) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state)
    : ATNConfig(state, other.alt, other.context, other.reachesIntoOuterContext, other.semanticContext, other.lexerActionExecutor) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state, AnySemanticContext semanticContext)
    : ATNConfig(state, other.alt, other.context, other.reachesIntoOuterContext, std::move(semanticContext), other.lexerActionExecutor) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state, LexerActionExecutor lexerActionExecutor)
    : ATNConfig(state, other.alt, other.context, other.reachesIntoOuterContext, other.semanticContext, std::move(lexerActionExecutor)) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state, AnyPredictionContext context)
    : ATNConfig(state, other.alt, std::move(context), other.reachesIntoOuterContext, other.semanticContext, other.lexerActionExecutor) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state, AnyPredictionContext context, AnySemanticContext semanticContext)
    : ATNConfig(state, other.alt, std::move(context), other.reachesIntoOuterContext, std::move(semanticContext), other.lexerActionExecutor) {}

ATNConfig::ATNConfig(ATNConfig const& other, ATNState *state, AnyPredictionContext context, LexerActionExecutor lexerActionExecutor)
    : ATNConfig(state, other.alt, std::move(context), other.reachesIntoOuterContext, other.semanticContext, std::move(lexerActionExecutor)) {}

ATNConfig::ATNConfig(ATNState *state, size_t alt, AnyPredictionContext context, size_t reachesIntoOuterContext, AnySemanticContext semanticContext, LexerActionExecutor lexerActionExecutor)
    : state(state), alt(alt), context(std::move(context)), reachesIntoOuterContext(reachesIntoOuterContext), semanticContext(std::move(semanticContext)), lexerActionExecutor(std::move(lexerActionExecutor)) {}

size_t ATNConfig::getOuterContextDepth() const {
  return reachesIntoOuterContext & ~SUPPRESS_PRECEDENCE_FILTER;
}

bool ATNConfig::isPrecedenceFilterSuppressed() const {
  return (reachesIntoOuterContext & SUPPRESS_PRECEDENCE_FILTER) != 0;
}

void ATNConfig::setPrecedenceFilterSuppressed(bool value) {
  if (value) {
    reachesIntoOuterContext |= SUPPRESS_PRECEDENCE_FILTER;
  } else {
    reachesIntoOuterContext &= ~SUPPRESS_PRECEDENCE_FILTER;
  }
}

bool ATNConfig::hasPassedThroughNonGreedyDecision() const {
  if (state != nullptr) {
    switch (state->getStateType()) {
      case ATNState::BLOCK_START:
      case ATNState::PLUS_BLOCK_START:
      case ATNState::STAR_BLOCK_START:
      case ATNState::PLUS_LOOP_BACK:
      case ATNState::STAR_LOOP_ENTRY:
      case ATNState::TOKEN_START:
        return static_cast<DecisionState*>(state)->nonGreedy;
      default:
        break;
    }
  }
  return false;
}

size_t ATNConfig::hashCode() const {
  size_t hashCode = misc::MurmurHash::initialize(7);
  hashCode = misc::MurmurHash::update(hashCode, state->stateNumber);
  hashCode = misc::MurmurHash::update(hashCode, alt);
  hashCode = misc::MurmurHash::update(hashCode, context);
  hashCode = misc::MurmurHash::update(hashCode, semanticContext);
  hashCode = misc::MurmurHash::update(hashCode, isPrecedenceFilterSuppressed() ? 1 : 0);
  hashCode = misc::MurmurHash::update(hashCode, lexerActionExecutor);
  hashCode = misc::MurmurHash::update(hashCode, hasPassedThroughNonGreedyDecision() ? 1 : 0);
  hashCode = misc::MurmurHash::finish(hashCode, 7);
  return hashCode;
}

bool ATNConfig::equals(const ATNConfig &other) const {
  return state->stateNumber == other.state->stateNumber && alt == other.alt &&
    context == other.context &&
    semanticContext == other.semanticContext &&
    isPrecedenceFilterSuppressed() == other.isPrecedenceFilterSuppressed() &&
    lexerActionExecutor == other.lexerActionExecutor &&
    hasPassedThroughNonGreedyDecision() == other.hasPassedThroughNonGreedyDecision();
}

std::string ATNConfig::toString() const {
  return toString(true);
}

std::string ATNConfig::toString(bool showAlt) const {
  std::stringstream ss;
  ss << "(";

  ss << state->toString();
  if (showAlt) {
    ss << "," << alt;
  }
  if (context) {
    ss << ",[" << context.toString() << "]";
  }
  if (semanticContext.valid() && semanticContext != SemanticContext::none()) {
    ss << ",[" << semanticContext.toString() << "]";
  }
  if (getOuterContextDepth() > 0) {
    ss << ",up=" << getOuterContextDepth();
  }
  ss << ")";

  return ss.str();
}
