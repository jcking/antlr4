/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "Token.h"
#include "misc/IntervalSet.h"

#include "atn/SetTransition.h"

using namespace antlr4;
using namespace antlr4::atn;

SetTransition::SetTransition(ATNState *target, misc::IntervalSet set)
    : Transition(target), _set(set.isEmpty() ? misc::IntervalSet::of(Token::INVALID_TYPE) : std::move(set)) {}

TransitionType SetTransition::getType() const {
  return TransitionType::SET;
}

const misc::IntervalSet& SetTransition::label() const {
  return _set;
}

bool SetTransition::matches(size_t symbol, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return _set.contains(symbol);
}

std::string SetTransition::toString() const {
  return "SET " + Transition::toString() + " { set: " + _set.toString() + "}";
}
