/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/RuleStartState.h"
#include "atn/RuleTransition.h"
#include "support/Casts.h"

using namespace antlr4::atn;
using namespace antlrcpp;

RuleTransition::RuleTransition(RuleStartState *ruleStart, size_t ruleIndex, int precedence, ATNState *followState)
    : Transition(ruleStart), _ruleIndex(ruleIndex), _followState(followState), _precedence(precedence) {}

TransitionType RuleTransition::getType() const {
  return TransitionType::RULE;
}

bool RuleTransition::isEpsilon() const {
  return true;
}

bool RuleTransition::matches(size_t /*symbol*/, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return false;
}

bool RuleTransition::equals(const Transition &other) const {
  if (getType() != other.getType()) {
    return false;
  }
  const RuleTransition &that = downCast<const RuleTransition&>(other);
  return getRuleIndex() == that.getRuleIndex() && getPrecedence() == that.getPrecedence() && getFollowState() == that.getFollowState() && Transition::equals(other);
}

std::string RuleTransition::toString() const {
  std::stringstream ss;
  ss << "RULE " << Transition::toString() << " { ruleIndex: " << getRuleIndex() << ", precedence: " << getPrecedence() <<
    ", followState: " << std::hex << getFollowState() << " }";
  return ss.str();
}
