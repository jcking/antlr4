/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

namespace antlr4 {
namespace atn {

  enum class PredictionContextType {
    EMPTY = 0,
    SINGLETON = 1,
    ARRAY = 2,
  };

} // namespace atn
} // namespace antlr4
