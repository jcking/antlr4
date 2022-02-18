/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC EpsilonTransition final : public Transition {
  public:
    explicit EpsilonTransition(ATNState *target);

    EpsilonTransition(ATNState *target, size_t outermostPrecedenceReturn);

    EpsilonTransition(const EpsilonTransition&) = default;

    EpsilonTransition(EpsilonTransition&&) = default;

    EpsilonTransition& operator=(const EpsilonTransition&) = default;

    EpsilonTransition& operator=(EpsilonTransition&&) = default;

    /**
     * @return the rule index of a precedence rule for which this transition is
     * returning from, where the precedence value is 0; otherwise, INVALID_INDEX.
     *
     * @see ATNConfig#isPrecedenceFilterSuppressed()
     * @see ParserATNSimulator#applyPrecedenceFilter(ATNConfigSet)
     * @since 4.4.1
     */
    constexpr size_t getOutermostPrecedenceReturn() const { return _outermostPrecedenceReturn; }

    virtual TransitionType getType() const override;

    virtual bool isEpsilon() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    bool equals(const Transition &other) const override;

    virtual std::string toString() const override;

  private:
    size_t _outermostPrecedenceReturn; // A rule index.
  };

} // namespace atn
} // namespace antlr4
