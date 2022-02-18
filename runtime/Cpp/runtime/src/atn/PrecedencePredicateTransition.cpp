/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/PrecedencePredicateTransition.h"
#include "support/Casts.h"

using namespace antlr4::atn;
using namespace antlrcpp;

PrecedencePredicateTransition::PrecedencePredicateTransition(ATNState *target, int precedence)
    : AbstractPredicateTransition(target), _precedence(precedence) {}

TransitionType PrecedencePredicateTransition::getType() const {
  return TransitionType::PRECEDENCE;
}

bool PrecedencePredicateTransition::isEpsilon() const {
  return true;
}

bool PrecedencePredicateTransition::matches(size_t /*symbol*/, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return false;
}

SemanticContext::PrecedencePredicate PrecedencePredicateTransition::getPredicate() const {
  return SemanticContext::PrecedencePredicate(getPrecedence());
}

bool PrecedencePredicateTransition::equals(const Transition &other) const {
  if (getType() != other.getType()) {
    return false;
  }
  const PrecedencePredicateTransition &that = downCast<const PrecedencePredicateTransition&>(other);
  return getPrecedence() == that.getPrecedence() && Transition::equals(other);
}

std::string PrecedencePredicateTransition::toString() const {
  return "PRECEDENCE " + Transition::toString() + " { precedence: " + std::to_string(getPrecedence()) + " }";
}
