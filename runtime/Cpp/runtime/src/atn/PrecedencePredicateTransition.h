/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/AbstractPredicateTransition.h"
#include "SemanticContext.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC PrecedencePredicateTransition final : public AbstractPredicateTransition {
  public:
    PrecedencePredicateTransition(ATNState *target, int precedence);

    PrecedencePredicateTransition(const PrecedencePredicateTransition&) = default;

    PrecedencePredicateTransition(PrecedencePredicateTransition&&) = default;

    PrecedencePredicateTransition& operator=(const PrecedencePredicateTransition&) = default;

    PrecedencePredicateTransition& operator=(PrecedencePredicateTransition&&) = default;

    constexpr int getPrecedence() const { return _precedence; }

    SemanticContext::PrecedencePredicate getPredicate() const;

    virtual TransitionType getType() const override;
    virtual bool isEpsilon() const override;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const override;

    bool equals(const Transition &other) const override;

    virtual std::string toString() const override;

  private:
    int _precedence;
  };

} // namespace atn
} // namespace antlr4
