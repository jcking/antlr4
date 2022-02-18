/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC RuleTransition final : public Transition {
  public:
    RuleTransition(RuleStartState *ruleStart, size_t ruleIndex, int precedence, ATNState *followState);

    RuleTransition(const RuleTransition&) = default;

    RuleTransition(RuleTransition&&) = default;

    RuleTransition& operator=(const RuleTransition&) = default;

    RuleTransition& operator=(RuleTransition&&) = default;

    constexpr size_t getRuleIndex() const { return _ruleIndex; }

    constexpr ATNState* getFollowState() const { return _followState; }

    constexpr int getPrecedence() const { return _precedence; }

    virtual TransitionType getType() const override;

    virtual bool isEpsilon() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    bool equals(const Transition &other) const override;

    virtual std::string toString() const override;

  private:
    size_t _ruleIndex;
    ATNState *_followState;
    int _precedence;
  };

} // namespace atn
} // namespace antlr4
