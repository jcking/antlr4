/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/Transition.h"

#include "Exceptions.h"
#include "support/Arrays.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlrcpp;

Transition::Transition(ATNState *target) {
  setTarget(target);
}

void Transition::setTarget(ATNState *target) {
  if (target == nullptr) {
    throw NullPointerException("target cannot be null.");
  }
  _target = target;
}

bool Transition::isEpsilon() const {
  return false;
}

const misc::IntervalSet& Transition::label() const {
  return misc::IntervalSet::EMPTY_SET;
}

bool Transition::equals(const Transition &other) const {
  return getType() == other.getType() && getTarget() == other.getTarget() && isEpsilon() == other.isEpsilon() && label() == other.label();
}

std::string Transition::toString() const {
  std::stringstream ss;
  ss << "(Transition " << std::hex << this << ", target: " << std::hex << getTarget() << ')';

  return ss.str();
}
