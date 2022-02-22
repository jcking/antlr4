
/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/PredictionContext.h"

namespace antlr4 {
namespace atn {

  class SingletonPredictionContext;
  class AnyPredictionContext;

  class ANTLR4CPP_PUBLIC ArrayPredictionContext final : public PredictionContext {
  public:
    explicit ArrayPredictionContext(const SingletonPredictionContext &other);

    explicit ArrayPredictionContext(std::vector<std::pair<AnyPredictionContext, size_t>> pairs);

    ArrayPredictionContext(const ArrayPredictionContext&) = default;

    ArrayPredictionContext(ArrayPredictionContext&&) = default;

    ArrayPredictionContext& operator=(const ArrayPredictionContext&) = default;

    ArrayPredictionContext& operator=(ArrayPredictionContext&&) = default;

    PredictionContextType getType() const override;

    virtual bool isEmpty() const override;
    virtual size_t size() const override;
    virtual const AnyPredictionContext& getParent(size_t index) const override;
    virtual size_t getReturnState(size_t index) const override;

  private:
    std::shared_ptr<const std::vector<std::pair<AnyPredictionContext, size_t>>> _pairs;
  };

} // namespace atn
} // namespace antlr4

