/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC RangeTransition final : public Transition {
  public:
    RangeTransition(ATNState *target, size_t from, size_t to);

    RangeTransition(const RangeTransition&) = default;

    RangeTransition(RangeTransition&&) = default;

    RangeTransition& operator=(const RangeTransition&) = default;

    RangeTransition& operator=(RangeTransition&&) = default;

    size_t getFrom() const { return static_cast<size_t>(_range.getMinElement()); }

    size_t getTo() const { return static_cast<size_t>(_range.getMaxElement()); }

    virtual TransitionType getType() const override;

    virtual const misc::IntervalSet& label() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    virtual std::string toString() const override;

  private:
    misc::IntervalSet _range;
  };

} // namespace atn
} // namespace antlr4
