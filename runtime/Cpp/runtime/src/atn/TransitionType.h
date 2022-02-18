/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <string_view>

namespace antlr4 {
namespace atn {

  enum class TransitionType {
    EPSILON = 1,
    RANGE = 2,
    RULE = 3,
    PREDICATE = 4, // e.g., {isType(input.LT(1))}?
    ATOM = 5,
    ACTION = 6,
    SET = 7, // ~(A|B) or ~atom, wildcard, which convert to next 2
    NOT_SET = 8,
    WILDCARD = 9,
    PRECEDENCE = 10,
  };

  std::string_view transitionName(TransitionType transitionType);

} // namespace atn
} // namespace antlr4
