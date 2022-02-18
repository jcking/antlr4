/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC ActionTransition final : public Transition {
  public:
    ActionTransition(ATNState *target, size_t ruleIndex);

    ActionTransition(ATNState *target, size_t ruleIndex, size_t actionIndex, bool isCtxDependent);

    ActionTransition(const ActionTransition&) = default;

    ActionTransition(ActionTransition&&) = default;

    ActionTransition& operator=(const ActionTransition&) = default;

    ActionTransition& operator=(ActionTransition&&) = default;

    constexpr size_t getRuleIndex() const { return _ruleIndex; }

    constexpr size_t getActionIndex() const { return _actionIndex; }

    constexpr bool isCtxDependent() const { return _isCtxDependent; }

    virtual TransitionType getType() const override;

    virtual bool isEpsilon() const override;

    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    bool equals(const Transition &other) const override;

    virtual std::string toString() const override;

  private:
    size_t _ruleIndex;
    size_t _actionIndex;
    bool _isCtxDependent; // e.g., $i ref in action
  };

} // namespace atn
} // namespace antlr4
