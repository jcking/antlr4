/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "support/Arrays.h"
#include "atn/SingletonPredictionContext.h"
#include "atn/AnyPredictionContext.h"

#include "atn/ArrayPredictionContext.h"

using namespace antlr4::atn;

ArrayPredictionContext::ArrayPredictionContext(const SingletonPredictionContext &other) {
  auto pairs = std::make_shared<std::vector<std::pair<AnyPredictionContext, size_t>>>();
  pairs->reserve(1);
  pairs->emplace_back(other.getParent(0), other.getReturnState(0));
  pairs->shrink_to_fit();
  _pairs = std::move(pairs);
}

ArrayPredictionContext::ArrayPredictionContext(std::vector<std::pair<AnyPredictionContext, size_t>> pairs) {
  pairs.shrink_to_fit();
  assert(pairs.size() > 0);
  _pairs = std::make_shared<std::vector<std::pair<AnyPredictionContext, size_t>>>(std::move(pairs));
}

PredictionContextType ArrayPredictionContext::getType() const {
  return PredictionContextType::ARRAY;
}

bool ArrayPredictionContext::isEmpty() const {
  // Since EMPTY_RETURN_STATE can only appear in the last position, we don't need to verify that size == 1.
  return getReturnState(0) == EMPTY_RETURN_STATE;
}

size_t ArrayPredictionContext::size() const {
  return _pairs->size();
}

const AnyPredictionContext& ArrayPredictionContext::getParent(size_t index) const {
  return (*_pairs)[index].first;
}

size_t ArrayPredictionContext::getReturnState(size_t index) const {
  return (*_pairs)[index].second;
}
