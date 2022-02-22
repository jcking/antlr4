/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/ATNType.h"
#include "atn/ATNConfigSet.h"
#include "dfa/DFAState.h"
#include "atn/ATNDeserializer.h"
#include "atn/EmptyPredictionContext.h"

#include "atn/ATNSimulator.h"

using namespace antlr4;
using namespace antlr4::dfa;
using namespace antlr4::atn;

const Ref<DFAState> ATNSimulator::ERROR = std::make_shared<DFAState>(INT32_MAX);

ATNSimulator::ATNSimulator(const ATN &atn) : atn(atn) {}

void ATNSimulator::clearDFA() {
  throw UnsupportedOperationException("This ATN simulator does not support clearing the DFA.");
}
