/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/EpsilonTransition.h"
#include "support/Casts.h"

using namespace antlr4::atn;
using namespace antlrcpp;

EpsilonTransition::EpsilonTransition(ATNState *target) : EpsilonTransition(target, INVALID_INDEX) {}

EpsilonTransition::EpsilonTransition(ATNState *target, size_t outermostPrecedenceReturn)
  : Transition(target), _outermostPrecedenceReturn(outermostPrecedenceReturn) {}

TransitionType EpsilonTransition::getType() const {
  return TransitionType::EPSILON;
}

bool EpsilonTransition::isEpsilon() const {
  return true;
}

bool EpsilonTransition::matches(size_t /*symbol*/, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return false;
}

bool EpsilonTransition::equals(const Transition &other) const {
  if (getType() != other.getType()) {
    return false;
  }
  const EpsilonTransition &that = downCast<const EpsilonTransition&>(other);
  return getOutermostPrecedenceReturn() == that.getOutermostPrecedenceReturn() && Transition::equals(other);
}

std::string EpsilonTransition::toString() const {
  return "EPSILON " + Transition::toString() + " {}";
}
