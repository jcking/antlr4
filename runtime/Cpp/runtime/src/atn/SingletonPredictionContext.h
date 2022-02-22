/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/PredictionContext.h"

namespace antlr4 {
namespace atn {

  class AnyPredictionContext;

  class ANTLR4CPP_PUBLIC SingletonPredictionContext : public PredictionContext {
  public:
    static AnyPredictionContext create(AnyPredictionContext parent, size_t returnState);

    SingletonPredictionContext(AnyPredictionContext parent, size_t returnState);

    SingletonPredictionContext(const SingletonPredictionContext&) = default;

    SingletonPredictionContext(SingletonPredictionContext&&) = default;

    SingletonPredictionContext& operator=(const SingletonPredictionContext&) = default;

    SingletonPredictionContext& operator=(SingletonPredictionContext&&) = default;

    PredictionContextType getType() const override;

    virtual size_t size() const final;
    virtual const AnyPredictionContext& getParent(size_t index) const final;
    virtual size_t getReturnState(size_t index) const final;
    bool isEmpty() const override;

  private:
    // Usually a parent is linked via a weak ptr. Not so here as we have kinda reverse reference chain.
    // There are no child contexts stored here and often the parent context is left dangling when it's
    // owning ATNState is released. In order to avoid having this context released as well (leaving all other contexts
    // which got this one as parent with a null reference) we use a shared_ptr here instead, to keep those left alone
    // parent contexts alive.
    std::shared_ptr<const AnyPredictionContext> _parent;
    size_t _returnState;
  };

} // namespace atn
} // namespace antlr4
