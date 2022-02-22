/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/ATNConfigSet.h"
#include "atn/SemanticContext.h"
#include "atn/ATNConfig.h"
#include "misc/MurmurHash.h"

#include "dfa/DFAState.h"

using namespace antlr4::dfa;
using namespace antlr4::atn;

DFAState::PredPrediction::PredPrediction(AnySemanticContext pred, int alt) : pred(std::move(pred)), alt(alt) {}

std::string DFAState::PredPrediction::toString() const {
  return std::string("(") + pred.toString() + ", " + std::to_string(alt) + ")";
}

DFAState::DFAState(int state) : DFAState() {
  stateNumber = state;
}

DFAState::DFAState(ATNConfigSet configs) : DFAState() {
  this->configs = std::move(configs);
}

std::set<size_t> DFAState::getAltSet() const {
  std::set<size_t> alts;
  for (const auto &config : configs) {
    alts.insert(config.alt);
  }
  return alts;
}

size_t DFAState::hashCode() const {
  size_t hash = misc::MurmurHash::initialize(7);
  hash = misc::MurmurHash::update(hash, configs);
  hash = misc::MurmurHash::finish(hash, 1);
  return hash;
}

bool DFAState::equals(const DFAState &other) const {
  // compare set of ATN configurations in this set with other
  if (this == &other) {
    return true;
  }

  return configs == other.configs;
}

std::string DFAState::toString() const {
  std::stringstream ss;
  ss << stateNumber;
  if (!configs.isEmpty()) {
    ss << ":" << configs.toString();
  }
  if (isAcceptState) {
    ss << " => ";
    if (!predicates.empty()) {
      for (size_t i = 0; i < predicates.size(); i++) {
        ss << predicates[i].toString();
      }
    } else {
      ss << prediction;
    }
  }
  return ss.str();
}
