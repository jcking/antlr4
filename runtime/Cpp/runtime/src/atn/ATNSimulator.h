/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/ATN.h"
#include "misc/IntervalSet.h"
#include "support/CPPUtils.h"
#include "atn/PredictionContext.h"
#include "atn/AnyTransition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC ATNSimulator {
  public:
    /// Must distinguish between missing edge and edge we know leads nowhere.
    static const Ref<dfa::DFAState> ERROR;
    const ATN &atn;

    explicit ATNSimulator(const ATN &atn);

    virtual ~ATNSimulator() = default;

    virtual void reset() = 0;

    /**
     * Clear the DFA cache used by the current instance. Since the DFA cache may
     * be shared by multiple ATN simulators, this method may affect the
     * performance (but not accuracy) of other parsers which are being used
     * concurrently.
     *
     * @throws UnsupportedOperationException if the current instance does not
     * support clearing the DFA.
     *
     * @since 4.3
     */
    virtual void clearDFA();
  };

} // namespace atn
} // namespace antlr4
