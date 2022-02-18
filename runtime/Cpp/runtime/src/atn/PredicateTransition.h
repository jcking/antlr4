/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/AbstractPredicateTransition.h"
#include "SemanticContext.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC PredicateTransition final : public AbstractPredicateTransition {
  public:
    PredicateTransition(ATNState *target, size_t ruleIndex, size_t predIndex, bool isCtxDependent);

    PredicateTransition(const PredicateTransition&) = default;

    PredicateTransition(PredicateTransition&&) = default;

    PredicateTransition& operator=(const PredicateTransition&) = default;

    PredicateTransition& operator=(PredicateTransition&&) = default;

    constexpr size_t getRuleIndex() const { return _ruleIndex; }

    constexpr size_t getPredIndex() const { return _predIndex; }

    constexpr bool isCtxDependent() const { return _isCtxDependent; }

    SemanticContext::Predicate getPredicate() const;

    virtual TransitionType getType() const override;

    virtual bool isEpsilon() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    bool equals(const Transition &other) const override;

    virtual std::string toString() const override;

  private:
    size_t _ruleIndex;
    size_t _predIndex;
    bool _isCtxDependent; // e.g., $i ref in pred
  };

} // namespace atn
} // namespace antlr4
