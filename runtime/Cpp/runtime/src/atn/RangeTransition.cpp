/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "misc/IntervalSet.h"

#include "atn/RangeTransition.h"

using namespace antlr4;
using namespace antlr4::atn;

RangeTransition::RangeTransition(ATNState *target, size_t from, size_t to) : Transition(target), _range(misc::IntervalSet::of(from, to)) {}

TransitionType RangeTransition::getType() const {
  return TransitionType::RANGE;
}

const misc::IntervalSet& RangeTransition::label() const {
  return _range;
}

bool RangeTransition::matches(size_t symbol, size_t /*minVocabSymbol*/, size_t /*maxVocabSymbol*/) const {
  return symbol >= getFrom() && symbol <= getTo();
}

std::string RangeTransition::toString() const {
  return "RANGE " + Transition::toString() + " { from: " + std::to_string(_range.getMinElement()) + ", to: " + std::to_string(_range.getMaxElement()) + " }";
}
