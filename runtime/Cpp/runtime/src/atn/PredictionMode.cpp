/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/RuleStopState.h"
#include "atn/ATNConfigSet.h"
#include "atn/ATN.h"
#include "atn/ATNConfig.h"
#include "misc/MurmurHash.h"
#include "SemanticContext.h"
#include "AnySemanticContext.h"

#include "PredictionMode.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlrcpp;

struct AltAndContextConfigHasher
{
  /**
   * The hash code is only a function of the {@link ATNState#stateNumber}
   * and {@link ATNConfig#context}.
   */
  size_t operator () (const ATNConfig *o) const {
    size_t hashCode = misc::MurmurHash::initialize(7);
    hashCode = misc::MurmurHash::update(hashCode, o->state->stateNumber);
    hashCode = misc::MurmurHash::update(hashCode, o->context);
    return misc::MurmurHash::finish(hashCode, 2);
  }
};

struct AltAndContextConfigComparer {
  bool operator()(const ATNConfig *a, const ATNConfig *b) const
  {
    if (a == b) {
      return true;
    }
    return a->state->stateNumber == b->state->stateNumber && a->context == b->context;
  }
};

bool PredictionModeClass::hasSLLConflictTerminatingPrediction(PredictionMode mode, const ATNConfigSet &configs) {
  /* Configs in rule stop states indicate reaching the end of the decision
   * rule (local context) or end of start rule (full context). If all
   * configs meet this condition, then none of the configurations is able
   * to match additional input so we terminate prediction.
   */
  if (allConfigsInRuleStopStates(configs)) {
    return true;
  }

  bool heuristic;

  // Pure SLL mode parsing or SLL+LL if:
  // Don't bother with combining configs from different semantic
  // contexts if we can fail over to full LL; costs more time
  // since we'll often fail over anyway.
  if (mode == PredictionMode::SLL || !configs.hasSemanticContext) {
    std::vector<antlrcpp::BitSet> altsets = getConflictingAltSubsets(configs);
    heuristic = hasConflictingAltSet(altsets) && !hasStateAssociatedWithOneAlt(configs);
  } else {
    // dup configs, tossing out semantic predicates
    ATNConfigSet dup(true);
    for (const auto &config : configs) {
      dup.add(ATNConfig(config, SemanticContext::none()));
    }
    std::vector<antlrcpp::BitSet> altsets = getConflictingAltSubsets(dup);
    heuristic = hasConflictingAltSet(altsets) && !hasStateAssociatedWithOneAlt(dup);
  }

  return heuristic;
}

bool PredictionModeClass::hasConfigInRuleStopState(const ATNConfigSet &configs) {
  for (const auto &config : configs) {
    if (config.state->getStateType() == ATNState::RULE_STOP) {
      return true;
    }
  }

  return false;
}

bool PredictionModeClass::allConfigsInRuleStopStates(const ATNConfigSet &configs) {
  for (const auto &config : configs) {
    if (config.state->getStateType() != ATNState::RULE_STOP) {
      return false;
    }
  }

  return true;
}

size_t PredictionModeClass::resolvesToJustOneViableAlt(const std::vector<antlrcpp::BitSet>& altsets) {
  return getSingleViableAlt(altsets);
}

bool PredictionModeClass::allSubsetsConflict(const std::vector<antlrcpp::BitSet>& altsets) {
  return !hasNonConflictingAltSet(altsets);
}

bool PredictionModeClass::hasNonConflictingAltSet(const std::vector<antlrcpp::BitSet>& altsets) {
  for (const auto &alts : altsets) {
    if (alts.count() == 1) {
      return true;
    }
  }
  return false;
}

bool PredictionModeClass::hasConflictingAltSet(const std::vector<antlrcpp::BitSet>& altsets) {
  for (const auto &alts : altsets) {
    if (alts.count() > 1) {
      return true;
    }
  }
  return false;
}

bool PredictionModeClass::allSubsetsEqual(const std::vector<antlrcpp::BitSet>& altsets) {
  if (altsets.empty()) {
    return true;
  }

  const antlrcpp::BitSet& first = *altsets.begin();
  for (const auto& alts : altsets) {
    if (alts != first) {
      return false;
    }
  }
  return true;
}

size_t PredictionModeClass::getUniqueAlt(const std::vector<antlrcpp::BitSet>& altsets) {
  antlrcpp::BitSet all = getAlts(altsets);
  if (all.count() == 1) {
    return all.find().value_or(INVALID_INDEX);
  }
  return ATN::INVALID_ALT_NUMBER;
}

antlrcpp::BitSet PredictionModeClass::getAlts(const std::vector<antlrcpp::BitSet>& altsets) {
  antlrcpp::BitSet all;
  for (antlrcpp::BitSet alts : altsets) {
    all |= alts;
  }

  return all;
}

antlrcpp::BitSet PredictionModeClass::getAlts(const ATNConfigSet &configs) {
  antlrcpp::BitSet alts;
  for (const auto &config : configs) {
    alts.set(config.alt);
  }
  return alts;
}

std::vector<antlrcpp::BitSet> PredictionModeClass::getConflictingAltSubsets(const ATNConfigSet &configs) {
  std::unordered_map<const ATNConfig*, antlrcpp::BitSet, AltAndContextConfigHasher, AltAndContextConfigComparer> configToAlts;
  for (const auto &config : configs) {
    configToAlts[&config].set(config.alt);
  }
  std::vector<antlrcpp::BitSet> values;
  values.reserve(configToAlts.size());
  for (auto &it : configToAlts) {
    values.push_back(std::move(it.second));
  }
  return values;
}

std::unordered_map<const ATNState*, antlrcpp::BitSet> PredictionModeClass::getStateToAltMap(const ATNConfigSet &configs) {
  std::unordered_map<const ATNState*, antlrcpp::BitSet> m;
  for (auto &config : configs) {
    m[config.state].set(config.alt);
  }
  return m;
}

bool PredictionModeClass::hasStateAssociatedWithOneAlt(const ATNConfigSet &configs) {
  auto x = getStateToAltMap(configs);
  for (auto it = x.begin(); it != x.end(); it++){
    if (it->second.count() == 1) return true;
  }
  return false;
}

size_t PredictionModeClass::getSingleViableAlt(const std::vector<antlrcpp::BitSet>& altsets) {
  antlrcpp::BitSet viableAlts;
  for (const auto &alts : altsets) {
    size_t minAlt = alts.find().value_or(INVALID_INDEX);

    viableAlts.set(minAlt);
    if (viableAlts.count() > 1)  // more than 1 viable alt
    {
      return ATN::INVALID_ALT_NUMBER;
    }
  }

  return viableAlts.find().value_or(INVALID_INDEX);
}
