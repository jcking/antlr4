/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/PredictionContext.h"
#include "atn/ATNConfig.h"
#include "atn/ATNSimulator.h"
#include "Exceptions.h"
#include "atn/SemanticContext.h"
#include "atn/AnyPredictionContext.h"
#include "support/Arrays.h"

#include "atn/ATNConfigSet.h"

using namespace antlr4::atn;
using namespace antlrcpp;

ATNConfigSet::ATNConfigSet() : ATNConfigSet(true) {}

ATNConfigSet::ATNConfigSet(bool fullCtx) : ATNConfigSet(fullCtx, false) {}

bool ATNConfigSet::add(const ATNConfig& config) {
  if (_readonly) {
    throw IllegalStateException("This set is readonly");
  }
  if (config.semanticContext != SemanticContext::none()) {
    hasSemanticContext = true;
  }
  if (config.getOuterContextDepth() > 0) {
    dipsIntoOuterContext = true;
  }

  auto [existing, inserted] = _configLookup.insert(config);
  if (inserted) {
    _cachedHashCode = 0;
    return true;
  }

  // a previous (s,i,pi,_), merge with it and save result
  bool rootIsWildcard = !fullCtx;
  AnyPredictionContext merged = PredictionContext::merge(existing->context, config.context, rootIsWildcard);
  // no need to check for existing.context, config.context in cache
  // since only way to create new graphs is "call rule" and here. We
  // cache at both places.
  const_cast<ATNConfig&>(*existing).reachesIntoOuterContext = std::max(existing->reachesIntoOuterContext, config.reachesIntoOuterContext);

  // make sure to preserve the precedence filter suppression during the merge
  if (config.isPrecedenceFilterSuppressed()) {
    const_cast<ATNConfig&>(*existing).setPrecedenceFilterSuppressed(true);
  }

  const_cast<ATNConfig&>(*existing).context = std::move(merged); // replace context; no need to alt mapping

  return true;
}

bool ATNConfigSet::addAll(const ATNConfigSet &other) {
  for (const auto &config : other) {
    add(config);
  }
  return false;
}

void ATNConfigSet::reserve(size_t size) {
  _configLookup.reserve(size);
}

std::vector<ATNState*> ATNConfigSet::getStates() const {
  std::vector<ATNState*> states;
  states.reserve(_configLookup.size());
  for (const auto &config : *this) {
    states.push_back(config.state);
  }
  return states;
}

/**
 * Gets the complete set of represented alternatives for the configuration
 * set.
 *
 * @return the set of represented alternatives in this configuration set
 *
 * @since 4.3
 */

BitSet ATNConfigSet::getAlts() const {
  BitSet alts;
  for (const auto &config : *this) {
    alts.set(config.alt);
  }
  return alts;
}

std::vector<AnySemanticContext> ATNConfigSet::getPredicates() const {
  std::vector<AnySemanticContext> preds;
  preds.reserve(_configLookup.size());
  for (const auto &config : *this) {
    if (config.semanticContext != SemanticContext::none()) {
      preds.push_back(config.semanticContext);
    }
  }
  return preds;
}

void ATNConfigSet::optimizeConfigs(ATNSimulator *interpreter) {
  assert(interpreter);

  if (_readonly) {
    throw IllegalStateException("This set is readonly");
  }
  if (_configLookup.empty())
    return;
}

bool ATNConfigSet::equals(const ATNConfigSet &other) const {
  if (&other == this) {
    return true;
  }

  if (_configLookup.size() != other._configLookup.size())
    return false;

  if (fullCtx != other.fullCtx || uniqueAlt != other.uniqueAlt ||
      conflictingAlts != other.conflictingAlts || hasSemanticContext != other.hasSemanticContext ||
      dipsIntoOuterContext != other.dipsIntoOuterContext) // includes stack context
    return false;

  return _configLookup == other._configLookup;
}

size_t ATNConfigSet::hashCode() const {
  size_t cachedHashCode = _cachedHashCode;
  if (!isReadonly() || cachedHashCode == 0) {
    cachedHashCode = 1;
    for (const auto &config : *this) {
      cachedHashCode = 31 * cachedHashCode + config.hashCode(); // Same as Java's list hashCode impl.
    }
    _cachedHashCode = cachedHashCode;
  }
  return cachedHashCode;
}

size_t ATNConfigSet::size() const {
  return _configLookup.size();
}

bool ATNConfigSet::isEmpty() const {
  return _configLookup.empty();
}

void ATNConfigSet::clear() {
  if (_readonly) {
    throw IllegalStateException("This set is readonly");
  }
  _cachedHashCode = 0;
  _configLookup.clear();
}

bool ATNConfigSet::isReadonly() const {
  return _readonly;
}

void ATNConfigSet::setReadonly(bool readonly) {
  _readonly = readonly;
}

ATNConfigSet::ATNConfigSet(bool fullCtx, bool ordered)
    : fullCtx(fullCtx), _configLookup(Container().bucket_count(), ATNConfigHasher{ordered}, ATNConfigComparer{ordered}) {}

std::string ATNConfigSet::toString() const {
  std::stringstream ss;
  ss << "[";
  for (const auto &config : *this) {
    ss << config.toString();
  }
  ss << "]";

  if (hasSemanticContext) {
    ss << ",hasSemanticContext = " <<  hasSemanticContext;
  }
  if (uniqueAlt != ATN::INVALID_ALT_NUMBER) {
    ss << ",uniqueAlt = " << uniqueAlt;
  }

  if (conflictingAlts.count() > 0) {
    ss << ",conflictingAlts = ";
    ss << conflictingAlts.toString();
  }

  if (dipsIntoOuterContext) {
    ss << ", dipsIntoOuterContext";
  }
  return ss.str();
}

size_t ATNConfigSet::hashCode(bool ordered, const ATNConfig &other) {
  if (ordered) {
    return other.hashCode();
  }
  size_t hashCode = 7;
  hashCode = 31 * hashCode + other.state->stateNumber;
  hashCode = 31 * hashCode + other.alt;
  hashCode = 31 * hashCode + other.semanticContext.hashCode();
  return hashCode;
}

bool ATNConfigSet::equals(bool ordered, const ATNConfig &lhs, const ATNConfig &rhs) {
  if (ordered) {
    return lhs == rhs;
  }
  return lhs.state->stateNumber == rhs.state->stateNumber && lhs.alt == rhs.alt && lhs.semanticContext == rhs.semanticContext;
}
