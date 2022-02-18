/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  /// <summary>
  /// A transition containing a set of values. </summary>
  class ANTLR4CPP_PUBLIC SetTransition : public Transition {
  public:
    SetTransition(ATNState *target, misc::IntervalSet set);

    SetTransition(const SetTransition&) = default;

    SetTransition(SetTransition&&) = default;

    SetTransition& operator=(const SetTransition&) = default;

    SetTransition& operator=(SetTransition&&) = default;

    virtual TransitionType getType() const override;

    virtual const misc::IntervalSet& label() const final;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    virtual std::string toString() const override;

  private:
    misc::IntervalSet _set;
  };

} // namespace atn
} // namespace antlr4
