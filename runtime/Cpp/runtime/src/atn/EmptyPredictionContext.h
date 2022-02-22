/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/SingletonPredictionContext.h"

namespace antlr4 {
namespace atn {

  class AnyPredictionContext;

  class ANTLR4CPP_PUBLIC EmptyPredictionContext final : public SingletonPredictionContext {
  public:
    EmptyPredictionContext();

    EmptyPredictionContext(const EmptyPredictionContext&) = default;

    EmptyPredictionContext(EmptyPredictionContext&&) = default;

    EmptyPredictionContext& operator=(const EmptyPredictionContext&) = default;

    EmptyPredictionContext& operator=(EmptyPredictionContext&&) = default;

    PredictionContextType getType() const override;

    bool isEmpty() const override;
  };

} // namespace atn
} // namespace antlr4
