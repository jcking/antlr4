/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/EmptyPredictionContext.h"
#include "misc/MurmurHash.h"
#include "atn/ArrayPredictionContext.h"
#include "atn/AnyPredictionContext.h"
#include "RuleContext.h"
#include "ParserRuleContext.h"
#include "atn/RuleTransition.h"
#include "support/Arrays.h"
#include "support/CPPUtils.h"
#include "atn/ATN.h"

#include "atn/PredictionContext.h"

using namespace antlr4;
using namespace antlr4::misc;
using namespace antlr4::atn;
using namespace antlrcpp;

AnyPredictionContext PredictionContext::empty() {
  return EmptyPredictionContext();
}

//----------------- PredictionContext ----------------------------------------------------------------------------------

AnyPredictionContext PredictionContext::fromRuleContext(const ATN &atn, RuleContext *outerContext) {
  if (outerContext == nullptr) {
    return PredictionContext::empty();
  }

  // if we are in RuleContext of start rule, s, then PredictionContext
  // is EMPTY. Nobody called us. (if we are empty, return empty)
  if (outerContext->parent == nullptr || outerContext == &ParserRuleContext::EMPTY) {
    return PredictionContext::empty();
  }

  // If we have a parent, convert it to a PredictionContext graph
  AnyPredictionContext parent = PredictionContext::fromRuleContext(atn, dynamic_cast<RuleContext*>(outerContext->parent));

  ATNState *state = atn.states.at(outerContext->invokingState).get();
  return SingletonPredictionContext::create(parent, state->transitions[0].as<RuleTransition>().getFollowState()->stateNumber);
}

bool PredictionContext::hasEmptyPath() const {
  // since EMPTY_RETURN_STATE can only appear in the last position, we check last one
  return getReturnState(size() - 1) == EMPTY_RETURN_STATE;
}

size_t PredictionContext::hashCode() const {
  size_t hash = MurmurHash::initialize(INITIAL_HASH);
  hash = MurmurHash::update(hash, static_cast<size_t>(getType()));
  for (size_t index = 0; index < size(); index++) {
    hash = MurmurHash::update(hash, getParent(index));
    hash = MurmurHash::update(hash, getReturnState(index));
  }
  return MurmurHash::finish(hash, size());
}

bool PredictionContext::equals(const PredictionContext &other) const {
  if (getType() != other.getType() || size() != other.size()) {
    return false;
  }
  for (size_t index = 0; index < size(); index++) {
    if (getParent(index) != other.getParent(index) || getReturnState(index) != other.getReturnState(index)) {
      return false;
    }
  }
  return true;
}

AnyPredictionContext PredictionContext::merge(const AnyPredictionContext &a,
  const AnyPredictionContext &b, bool rootIsWildcard) {
  assert(a && b);

  // share same graph if both same
  if (a == b) {
    return a;
  }

  if (a.is<SingletonPredictionContext>() && b.is<SingletonPredictionContext>()) {
    return mergeSingletons(a.as<SingletonPredictionContext>(), b.as<SingletonPredictionContext>(), rootIsWildcard);
  }

  // At least one of a or b is array.
  // If one is $ and rootIsWildcard, return $ as * wildcard.
  if (rootIsWildcard) {
    if (a.is<EmptyPredictionContext>()) {
      return a;
    }
    if (b.is<EmptyPredictionContext>()) {
      return b;
    }
  }

  // convert singleton so both are arrays to normalize
  AnyPredictionContext left;
  if (a.is<SingletonPredictionContext>()) {
    left = ArrayPredictionContext(a.as<SingletonPredictionContext>());
  } else {
    left = a.as<ArrayPredictionContext>();
  }
  AnyPredictionContext right;
  if (b.is<SingletonPredictionContext>()) {
    right = ArrayPredictionContext(b.as<SingletonPredictionContext>());
  } else {
    right = b.as<ArrayPredictionContext>();
  }
  return mergeArrays(left.as<ArrayPredictionContext>(), right.as<ArrayPredictionContext>(), rootIsWildcard);
}

AnyPredictionContext PredictionContext::mergeSingletons(const SingletonPredictionContext &a,
  const SingletonPredictionContext &b, bool rootIsWildcard) {
  AnyPredictionContext rootMerge = mergeRoot(a, b, rootIsWildcard);
  if (rootMerge.valid()) {
    return rootMerge;
  }

  const AnyPredictionContext &parentA = a.getParent(0);
  const AnyPredictionContext &parentB = b.getParent(0);
  if (a.getReturnState(0) == b.getReturnState(0)) { // a == b
    AnyPredictionContext parent = merge(parentA, parentB, rootIsWildcard);

    // If parent is same as existing a or b parent or reduced to a parent, return it.
    if (parent == parentA) { // ax + bx = ax, if a=b
      return a;
    }
    if (parent == parentB) { // ax + bx = bx, if a=b
      return b;
    }

    // else: ax + ay = a'[x,y]
    // merge parents x and y, giving array node with x,y then remainders
    // of those graphs.  dup a, a' points at merged array
    // new joined parent so create new singleton pointing to it, a'
    return SingletonPredictionContext::create(parent, a.getReturnState(0));
  }
  // a != b payloads differ
  // see if we can collapse parents due to $+x parents if local ctx
  AnyPredictionContext singleParent;
  if (a == b || (parentA == parentB)) { // ax + bx = [a,b]x
    singleParent = parentA;
  }
  if (singleParent.valid()) { // parents are same, sort payloads and use same parent
    std::vector<std::pair<AnyPredictionContext, size_t>> pairs;
    pairs.reserve(2);
    pairs.emplace_back(std::make_pair(singleParent, a.getReturnState(0)));
    pairs.emplace_back(std::make_pair(singleParent, b.getReturnState(0)));
    if (a.getReturnState(0) > b.getReturnState(0)) {
      pairs[0].second = b.getReturnState(0);
      pairs[1].second = a.getReturnState(0);
    }
    return ArrayPredictionContext(std::move(pairs));
  }

  // parents differ and can't merge them. Just pack together
  // into array; can't merge.
  // ax + by = [ax,by]
  AnyPredictionContext a_;
  if (a.getReturnState(0) > b.getReturnState(0)) { // sort by payload
    std::vector<std::pair<AnyPredictionContext, size_t>> pairs;
    pairs.reserve(2);
    pairs.emplace_back(std::make_pair(b.getParent(0), b.getReturnState(0)));
    pairs.emplace_back(std::make_pair(a.getParent(0), a.getReturnState(0)));
    return ArrayPredictionContext(std::move(pairs));
  }
  std::vector<std::pair<AnyPredictionContext, size_t>> pairs;
  pairs.reserve(2);
  pairs.emplace_back(std::make_pair(a.getParent(0), a.getReturnState(0)));
  pairs.emplace_back(std::make_pair(b.getParent(0), b.getReturnState(0)));
  return ArrayPredictionContext(std::move(pairs));
}

AnyPredictionContext PredictionContext::mergeRoot(const SingletonPredictionContext &a,
  const SingletonPredictionContext &b, bool rootIsWildcard) {
  if (rootIsWildcard) {
    if (a == PredictionContext::empty()) { // * + b = *
      return PredictionContext::empty();
    }
    if (b == PredictionContext::empty()) { // a + * = *
      return PredictionContext::empty();
    }
  } else {
    if (a == PredictionContext::empty() && b == PredictionContext::empty()) { // $ + $ = $
      return PredictionContext::empty();
    }
    if (a == PredictionContext::empty()) { // $ + x = [$,x]
      std::vector<std::pair<AnyPredictionContext, size_t>> pairs;
      pairs.reserve(2);
      pairs.emplace_back(std::make_pair(b.getParent(0), b.getReturnState(0)));
      pairs.emplace_back(std::make_pair(AnyPredictionContext(), EMPTY_RETURN_STATE));
      return ArrayPredictionContext(std::move(pairs));
    }
    if (b == PredictionContext::empty()) { // x + $ = [$,x] ($ is always first if present)
      std::vector<std::pair<AnyPredictionContext, size_t>> pairs;
      pairs.reserve(2);
      pairs.emplace_back(std::make_pair(a.getParent(0), a.getReturnState(0)));
      pairs.emplace_back(std::make_pair(AnyPredictionContext(), EMPTY_RETURN_STATE));
      return ArrayPredictionContext(std::move(pairs));
    }
  }
  return AnyPredictionContext();
}

AnyPredictionContext PredictionContext::mergeArrays(const ArrayPredictionContext &a,
  const ArrayPredictionContext &b, bool rootIsWildcard) {
  // merge sorted payloads a + b => M
  size_t i = 0; // walks a
  size_t j = 0; // walks b
  size_t k = 0; // walks target M array

  std::vector<std::pair<AnyPredictionContext, size_t>> mergedPairs(a.size() + b.size());

  // walk and merge to yield mergedParents, mergedReturnStates
  while (i < a.size() && j < b.size()) {
    const AnyPredictionContext &a_parent = a.getParent(i);
    const AnyPredictionContext &b_parent = b.getParent(j);
    if (a.getReturnState(i) == b.getReturnState(j)) {
      // same payload (stack tops are equal), must yield merged singleton
      size_t payload = a.getReturnState(i);
      // $+$ = $
      bool both = payload == EMPTY_RETURN_STATE && !a_parent && !b_parent;
      bool ax_ax = (a_parent && b_parent) && a_parent == b_parent; // ax+ax -> ax
      if (both || ax_ax) {
        mergedPairs[k].first = a_parent; // choose left
        mergedPairs[k].second = payload;
      }
      else { // ax+ay -> a'[x,y]
        mergedPairs[k].first = merge(a_parent, b_parent, rootIsWildcard);
        mergedPairs[k].second = payload;
      }
      i++; // hop over left one as usual
      j++; // but also skip one in right side since we merge
    } else if (a.getReturnState(i) < b.getReturnState(j)) { // copy a[i] to M
      mergedPairs[k].first = a_parent;
      mergedPairs[k].second = a.getReturnState(i);
      i++;
    }
    else { // b > a, copy b[j] to M
      mergedPairs[k].first = b_parent;
      mergedPairs[k].second = b.getReturnState(j);
      j++;
    }
    k++;
  }

  // copy over any payloads remaining in either array
  if (i < a.size()) {
    for (auto p = i; p < a.size(); p++) {
      mergedPairs[k].first = a.getParent(p);
      mergedPairs[k].second = a.getReturnState(p);
      k++;
    }
  } else {
    for (auto p = j; p < b.size(); p++) {
      mergedPairs[k].first = b.getParent(p);
      mergedPairs[k].second = b.getReturnState(p);
      k++;
    }
  }

  // trim merged if we combined a few that had same stack tops
  if (k < mergedPairs.size()) { // write index < last position; trim
    if (k == 1) { // for just one merged element, return singleton top
      return SingletonPredictionContext::create(mergedPairs[0].first, mergedPairs[0].second);
    }
    mergedPairs.resize(k);
  }

  ArrayPredictionContext M(std::move(mergedPairs));

  // if we created same array as a or b, return that instead
  // TODO: track whether this is possible above during merge sort for speed
  if (M == a) {
    return a;
  }
  if (M == b) {
    return b;
  }
  return M;
}

// The "visited" map is just a temporary structure to control the retrieval process (which is recursive).
AnyPredictionContext PredictionContext::getCachedContext(const AnyPredictionContext &context,
  std::unordered_map<AnyPredictionContext, AnyPredictionContext> &visited) {
  if (context.isEmpty()) {
    return context;
  }

  {
    auto iterator = visited.find(context);
    if (iterator != visited.end())
      return iterator->second; // Not necessarly the same as context.
  }

  bool changed = false;

  std::vector<std::pair<AnyPredictionContext, size_t>> pairs(context.size());
  for (size_t i = 0; i < pairs.size(); i++) {
    AnyPredictionContext parent = getCachedContext(context.getParent(i), visited);
    if (changed || parent != context.getParent(i)) {
      if (!changed) {
        pairs.clear();
        for (size_t j = 0; j < context.size(); j++) {
          pairs.push_back(std::make_pair(context.getParent(j), context.getReturnState(j)));
        }

        changed = true;
      }

      pairs[i] = std::make_pair(parent, context.getReturnState(i));
    }
  }

  if (!changed) {
    visited[context] = context;

    return context;
  }

  AnyPredictionContext updated;
  if (pairs.empty()) {
    updated = PredictionContext::empty();
  } else if (pairs.size() == 1) {
    updated = SingletonPredictionContext::create(pairs[0].first, pairs[0].second);
  } else {
    updated = ArrayPredictionContext(std::move(pairs));
  }

  visited[updated] = updated;
  visited[context] = updated;

  return updated;
}

std::vector<AnyPredictionContext> PredictionContext::getAllContextNodes(const AnyPredictionContext &context) {
  std::vector<AnyPredictionContext> nodes;
  std::unordered_set<AnyPredictionContext> visited;
  getAllContextNodes(context, nodes, visited);
  return nodes;
}


void PredictionContext::getAllContextNodes(const AnyPredictionContext &context, std::vector<AnyPredictionContext> &nodes,
  std::unordered_set<AnyPredictionContext> &visited) {

  if (visited.find(context) != visited.end()) {
    return; // Already done.
  }

  visited.insert(context);
  nodes.push_back(context);

  for (size_t i = 0; i < context.size(); i++) {
    getAllContextNodes(context.getParent(i), nodes, visited);
  }
}

std::string PredictionContext::toString() const {
  return antlrcpp::toString(this);
}

std::string PredictionContext::toString(Recognizer * /*recog*/) const {
  return toString();
}

std::vector<std::string> PredictionContext::toStrings(Recognizer *recognizer, int currentState) const {
  return toStrings(recognizer, PredictionContext::empty(), currentState);
}

std::vector<std::string> PredictionContext::toStrings(Recognizer *recognizer, const AnyPredictionContext &stop, int currentState) const {

  std::vector<std::string> result;

  for (size_t perm = 0; ; perm++) {
    size_t offset = 0;
    bool last = true;
    const PredictionContext *p = this;
    size_t stateNumber = currentState;

    std::stringstream ss;
    ss << "[";
    bool outerContinue = false;
    while (!p->isEmpty() && p != &stop.get()) {
      size_t index = 0;
      if (p->size() > 0) {
        size_t bits = 1;
        while ((1ULL << bits) < p->size()) {
          bits++;
        }

        size_t mask = (1 << bits) - 1;
        index = (perm >> offset) & mask;
        last &= index >= p->size() - 1;
        if (index >= p->size()) {
          outerContinue = true;
          break;
        }
        offset += bits;
      }

      if (recognizer != nullptr) {
        if (ss.tellp() > 1) {
          // first char is '[', if more than that this isn't the first rule
          ss << ' ';
        }

        const ATN &atn = recognizer->getATN();
        ATNState *s = atn.states[stateNumber].get();
        std::string ruleName = recognizer->getRuleNames()[s->ruleIndex];
        ss << ruleName;
      } else if (p->getReturnState(index) != EMPTY_RETURN_STATE) {
        if (!p->isEmpty()) {
          if (ss.tellp() > 1) {
            // first char is '[', if more than that this isn't the first rule
            ss << ' ';
          }

          ss << p->getReturnState(index);
        }
      }
      stateNumber = p->getReturnState(index);
      p = &p->getParent(index).get();
    }

    if (outerContinue)
      continue;

    ss << "]";
    result.push_back(ss.str());

    if (last) {
      break;
    }
  }

  return result;
}
