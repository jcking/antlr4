/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/PredicateTransition.h"
#include "support/Casts.h"

using namespace antlr4::atn;
using namespace antlrcpp;

PredicateTransition::PredicateTransition(ATNState *target, size_t ruleIndex, size_t predIndex, bool isCtxDependent)
    : AbstractPredicateTransition(target), _ruleIndex(ruleIndex), _predIndex(predIndex), _isCtxDependent(isCtxDependent) {}

TransitionType PredicateTransition::getType() const {
  return TransitionType::PREDICATE;
}

bool PredicateTransition::isEpsilon() const {
  return true;
}

bool PredicateTransition::matches(size_t /*symbol*/, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return false;
}

SemanticContext::Predicate PredicateTransition::getPredicate() const {
  return SemanticContext::Predicate(getRuleIndex(), getPredIndex(), isCtxDependent());
}

bool PredicateTransition::equals(const Transition &other) const {
  if (getType() != other.getType()) {
    return false;
  }
  const PredicateTransition &that = downCast<const PredicateTransition&>(other);
  return getRuleIndex() == that.getRuleIndex() && getPredIndex() == that.getPredIndex() && isCtxDependent() == that.isCtxDependent() && Transition::equals(other);
}

std::string PredicateTransition::toString() const {
  return "PREDICATE " + Transition::toString() + " { ruleIndex: " + std::to_string(getRuleIndex()) +
    ", predIndex: " + std::to_string(getPredIndex()) + ", isCtxDependent: " + std::to_string(isCtxDependent()) + " }";

  // Generate and add a predicate context here?
}
