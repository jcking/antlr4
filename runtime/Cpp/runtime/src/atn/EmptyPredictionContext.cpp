/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/EmptyPredictionContext.h"
#include "atn/AnyPredictionContext.h"

using namespace antlr4::atn;

EmptyPredictionContext::EmptyPredictionContext() : SingletonPredictionContext(AnyPredictionContext(), EMPTY_RETURN_STATE) {}

PredictionContextType EmptyPredictionContext::getType() const {
  return PredictionContextType::EMPTY;
}

bool EmptyPredictionContext::isEmpty() const {
  return true;
}
