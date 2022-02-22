/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/SingletonPredictionContext.h"
#include "atn/AnyPredictionContext.h"

using namespace antlr4::atn;

namespace {

const AnyPredictionContext invalidAnyPredictionContext;

}

SingletonPredictionContext::SingletonPredictionContext(AnyPredictionContext parent, size_t returnState)
  : _parent(parent ? std::make_shared<AnyPredictionContext>(std::move(parent)) : nullptr), _returnState(returnState) {
  assert(returnState != ATNState::INVALID_STATE_NUMBER);
}

AnyPredictionContext SingletonPredictionContext::create(AnyPredictionContext parent, size_t returnState) {
  if (returnState == EMPTY_RETURN_STATE && parent) {
    // someone can pass in the bits of an array ctx that mean $
    return empty();
  }
  return SingletonPredictionContext(std::move(parent), returnState);
}

size_t SingletonPredictionContext::size() const {
  return 1;
}

PredictionContextType SingletonPredictionContext::getType() const {
  return PredictionContextType::SINGLETON;
}

const AnyPredictionContext& SingletonPredictionContext::getParent(size_t index) const {
  static_cast<void>(index);
  assert(index == 0);
  return _parent ? *_parent : invalidAnyPredictionContext;
}

size_t SingletonPredictionContext::getReturnState(size_t index) const {
  static_cast<void>(index);
  assert(index == 0);
  return _returnState;
}

bool SingletonPredictionContext::isEmpty() const {
  return false;
}
