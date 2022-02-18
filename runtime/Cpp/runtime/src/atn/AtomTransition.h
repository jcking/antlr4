/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC AtomTransition final : public Transition {
  public:
    AtomTransition(ATNState *target, size_t label);

    AtomTransition(const AtomTransition&) = default;

    AtomTransition(AtomTransition&&) = default;

    AtomTransition& operator=(const AtomTransition&) = default;

    AtomTransition& operator=(AtomTransition&&) = default;

    virtual TransitionType getType() const override;

    virtual const misc::IntervalSet& label() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    virtual std::string toString() const override;

  private:
    // The token type or character value; or, signifies special label.
    misc::IntervalSet _label;
  };

} // namespace atn
} // namespace antlr4
