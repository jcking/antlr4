/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/ActionTransition.h"
#include "support/Casts.h"

using namespace antlr4::atn;
using namespace antlrcpp;

ActionTransition::ActionTransition(ATNState *target, size_t ruleIndex)
  : ActionTransition(target, ruleIndex, INVALID_INDEX, false) {}

ActionTransition::ActionTransition(ATNState *target, size_t ruleIndex, size_t actionIndex, bool isCtxDependent)
  : Transition(target), _ruleIndex(ruleIndex), _actionIndex(actionIndex), _isCtxDependent(isCtxDependent) {}

TransitionType ActionTransition::getType() const {
  return TransitionType::ACTION;
}

bool ActionTransition::isEpsilon() const {
  return true; // we are to be ignored by analysis 'cept for predicates
}

bool ActionTransition::matches(size_t /*symbol*/, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return false;
}

bool ActionTransition::equals(const Transition &other) const {
  if (getType() != other.getType()) {
    return false;
  }
  const ActionTransition &that = downCast<const ActionTransition&>(other);
  return getRuleIndex() == that.getRuleIndex() && getActionIndex() == that.getActionIndex() && isCtxDependent() == that.isCtxDependent() && Transition::equals(other);
}

std::string ActionTransition::toString() const {
  return " ACTION " + Transition::toString() + " { ruleIndex: " + std::to_string(getRuleIndex()) + ", actionIndex: " +
  std::to_string(getActionIndex()) + ", isCtxDependent: " + std::to_string(isCtxDependent()) + " }";
}
