/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "dfa/DFA.h"
#include "NoViableAltException.h"
#include "atn/DecisionState.h"
#include "ParserRuleContext.h"
#include "misc/IntervalSet.h"
#include "Parser.h"
#include "CommonTokenStream.h"
#include "atn/EmptyPredictionContext.h"
#include "atn/NotSetTransition.h"
#include "atn/AtomTransition.h"
#include "atn/RuleTransition.h"
#include "atn/PredicateTransition.h"
#include "atn/PrecedencePredicateTransition.h"
#include "atn/ActionTransition.h"
#include "atn/EpsilonTransition.h"
#include "atn/AnyTransition.h"
#include "atn/RuleStopState.h"
#include "atn/ATNConfigSet.h"
#include "atn/ATNConfig.h"

#include "atn/StarLoopEntryState.h"
#include "atn/BlockStartState.h"
#include "atn/BlockEndState.h"

#include "misc/Interval.h"
#include "ANTLRErrorListener.h"

#include "Vocabulary.h"
#include "support/Arrays.h"

#include "atn/ParserATNSimulator.h"

#define DEBUG_ATN 0
#define DEBUG_LIST_ATN_DECISIONS 0
#define DEBUG_DFA 0
#define RETRY_DEBUG 0

using namespace antlr4;
using namespace antlr4::atn;

using namespace antlrcpp;

const bool ParserATNSimulator::TURN_OFF_LR_LOOP_ENTRY_BRANCH_OPT = ParserATNSimulator::getLrLoopSetting();

ParserATNSimulator::ParserATNSimulator(const ATN &atn, std::vector<dfa::DFA> &decisionToDFA)
: ParserATNSimulator(nullptr, atn, decisionToDFA) {
}

ParserATNSimulator::ParserATNSimulator(Parser *parser, const ATN &atn, std::vector<dfa::DFA> &decisionToDFA)
: ATNSimulator(atn), decisionToDFA(decisionToDFA), parser(parser) {}

void ParserATNSimulator::reset() {
}

void ParserATNSimulator::clearDFA() {
  int size = (int)decisionToDFA.size();
  decisionToDFA.clear();
  for (int d = 0; d < size; ++d) {
    decisionToDFA.push_back(dfa::DFA(atn.getDecisionState(d), d));
  }
}

size_t ParserATNSimulator::adaptivePredict(TokenStream *input, size_t decision, ParserRuleContext *outerContext) {

#if DEBUG_ATN == 1 || DEBUG_LIST_ATN_DECISIONS == 1
    std::cout << "adaptivePredict decision " << decision << " exec LA(1)==" << getLookaheadName(input) << " line "
      << input->LT(1)->getLine() << ":" << input->LT(1)->getCharPositionInLine() << std::endl;
#endif

  _input = input;
  _startIndex = input->index();
  _outerContext = outerContext;
  dfa::DFA &dfa = decisionToDFA[decision];
  _dfa = &dfa;

  ssize_t m = input->mark();
  size_t index = _startIndex;

  // Now we are certain to have a specific decision's DFA
  // But, do we still need an initial state?
  auto onExit = finally([this, input, index, m] {
    _dfa = nullptr;
    input->seek(index);
    input->release(m);
  });

  dfa::DFAState *s0 = atn.getParserStartState(dfa, *parser);

  if (s0 == nullptr) {
    s0 = atn.updateParserStartState(dfa, computeStartState(dfa.atnStartState, &ParserRuleContext::EMPTY, false), parser, _outerContext);
  }

  // We can start with an existing DFA.
  size_t alt = execATN(dfa, s0, input, index, outerContext != nullptr ? outerContext : &ParserRuleContext::EMPTY);

  return alt;
}

size_t ParserATNSimulator::execATN(dfa::DFA &dfa, dfa::DFAState *s0, TokenStream *input, size_t startIndex,
                                   ParserRuleContext *outerContext) {

#if DEBUG_ATN == 1 || DEBUG_LIST_ATN_DECISIONS == 1
    std::cout << "execATN decision " << dfa.decision << " exec LA(1)==" << getLookaheadName(input) <<
      " line " << input->LT(1)->getLine() << ":" << input->LT(1)->getCharPositionInLine() << std::endl;
#endif

  dfa::DFAState *previousD = s0;

#if DEBUG_ATN == 1
    std::cout << "s0 = " << s0 << std::endl;
#endif

  size_t t = input->LA(1);

  while (true) { // while more work
    dfa::DFAState *D = getExistingTargetState(previousD, t);
    if (D == nullptr) {
      D = computeTargetState(dfa, previousD, t);
    }

    if (D == ERROR.get()) {
      // if any configs in previous dipped into outer context, that
      // means that input up to t actually finished entry rule
      // at least for SLL decision. Full LL doesn't dip into outer
      // so don't need special case.
      // We will get an error no matter what so delay until after
      // decision; better error message. Also, no reachable target
      // ATN states in SLL implies LL will also get nowhere.
      // If conflict in states that dip out, choose min since we
      // will get error no matter what.
      input->seek(startIndex);
      size_t alt = getSynValidOrSemInvalidAltThatFinishedDecisionEntryRule(previousD->configs, outerContext);
      if (alt != ATN::INVALID_ALT_NUMBER) {
        return alt;
      }

      throw noViableAlt(input, outerContext, previousD->configs, startIndex);
    }

    if (D->requiresFullContext && _mode != PredictionMode::SLL) {
      // IF PREDS, MIGHT RESOLVE TO SINGLE ALT => SLL (or syntax error)
      BitSet conflictingAlts;
      if (D->predicates.size() != 0) {
#if DEBUG_ATN == 1
          std::cout << "DFA state has preds in DFA sim LL failover" << std::endl;
#endif

        size_t conflictIndex = input->index();
        if (conflictIndex != startIndex) {
          input->seek(startIndex);
        }

        conflictingAlts = evalSemanticContext(D->predicates, outerContext, true);
        if (conflictingAlts.count() == 1) {
#if DEBUG_ATN == 1
            std::cout << "Full LL avoided" << std::endl;
#endif

          return conflictingAlts.find().value_or(INVALID_INDEX);
        }

        if (conflictIndex != startIndex) {
          // restore the index so reporting the fallback to full
          // context occurs with the index at the correct spot
          input->seek(conflictIndex);
        }
      }

#if DEBUG_DFA == 1
        std::cout << "ctx sensitive state " << outerContext << " in " << D << std::endl;
#endif

      bool fullCtx = true;
      ATNConfigSet s0_closure = computeStartState(dfa.atnStartState, outerContext, fullCtx);
      reportAttemptingFullContext(dfa, conflictingAlts, D->configs, startIndex, input->index());
      size_t alt = execATNWithFullContext(dfa, D, s0_closure, input, startIndex, outerContext);
      return alt;
    }

    if (D->isAcceptState) {
      if (D->predicates.empty()) {
        return D->prediction;
      }

      size_t stopIndex = input->index();
      input->seek(startIndex);
      BitSet alts = evalSemanticContext(D->predicates, outerContext, true);
      switch (alts.count()) {
        case 0:
          throw noViableAlt(input, outerContext, D->configs, startIndex);

        case 1:
          return alts.find().value_or(INVALID_INDEX);

        default:
          // report ambiguity after predicate evaluation to make sure the correct
          // set of ambig alts is reported.
          reportAmbiguity(dfa, D, startIndex, stopIndex, false, alts, D->configs);
          return alts.find().value_or(INVALID_INDEX);
      }
    }

    previousD = D;

    if (t != Token::EOF) {
      input->consume();
      t = input->LA(1);
    }
  }
}

dfa::DFAState *ParserATNSimulator::getExistingTargetState(dfa::DFAState *previousD, size_t t) {
  return atn.getParserExistingTargetState(*previousD, t);
}

dfa::DFAState *ParserATNSimulator::computeTargetState(dfa::DFA &dfa, dfa::DFAState *previousD, size_t t) {
  ATNConfigSet reach = computeReachSet(previousD->configs, t, false);
  if (reach.isEmpty()) {
    addDFAEdge(dfa, previousD, t, std::unique_ptr<dfa::DFAState>(ERROR.get()));
    return ERROR.get();
  }

  // create new target state; we'll add to DFA after it's complete
  auto D = std::make_unique<dfa::DFAState>(std::move(reach)); /* mem-check: managed by the DFA or deleted below, "reach" is no longer valid now. */
  size_t predictedAlt = getUniqueAlt(D->configs);

  if (predictedAlt != ATN::INVALID_ALT_NUMBER) {
    // NO CONFLICT, UNIQUELY PREDICTED ALT
    D->isAcceptState = true;
    D->configs.uniqueAlt = predictedAlt;
    D->prediction = predictedAlt;
  } else if (PredictionModeClass::hasSLLConflictTerminatingPrediction(_mode, D->configs)) {
    // MORE THAN ONE VIABLE ALTERNATIVE
    D->configs.conflictingAlts = getConflictingAlts(D->configs);
    D->requiresFullContext = true;
    // in SLL-only mode, we will stop at this state and return the minimum alt
    D->isAcceptState = true;
    D->prediction = D->configs.conflictingAlts.find().value_or(INVALID_INDEX);
  }

  if (D->isAcceptState && D->configs.hasSemanticContext) {
    predicateDFAState(D.get(), atn.getDecisionState(dfa.decision));
    if (D->predicates.size() != 0) {
      D->prediction = ATN::INVALID_ALT_NUMBER;
    }
  }

  // all adds to dfa are done after we've created full D state
  return addDFAEdge(dfa, previousD, t, std::move(D));
}

void ParserATNSimulator::predicateDFAState(dfa::DFAState *dfaState, DecisionState *decisionState) {
  // We need to test all predicates, even in DFA states that
  // uniquely predict alternative.
  size_t nalts = decisionState->transitions.size();

  // Update DFA so reach becomes accept state with (predicate,alt)
  // pairs if preds found for conflicting alts
  BitSet altsToCollectPredsFrom = getConflictingAltsOrUniqueAlt(dfaState->configs);
  std::vector<AnySemanticContext> altToPred = getPredsForAmbigAlts(altsToCollectPredsFrom, dfaState->configs, nalts);
  if (!altToPred.empty()) {
    dfaState->predicates = getPredicatePredictions(altsToCollectPredsFrom, altToPred);
    dfaState->prediction = ATN::INVALID_ALT_NUMBER; // make sure we use preds
  } else {
    // There are preds in configs but they might go away
    // when OR'd together like {p}? || NONE == NONE. If neither
    // alt has preds, resolve to min alt
    dfaState->prediction = altsToCollectPredsFrom.find().value_or(INVALID_INDEX);
  }
}

size_t ParserATNSimulator::execATNWithFullContext(dfa::DFA &dfa, dfa::DFAState *D, const ATNConfigSet &s0,
  TokenStream *input, size_t startIndex, ParserRuleContext *outerContext) {

  bool fullCtx = true;
  bool foundExactAmbig = false;

  ATNConfigSet reach;
  ATNConfigSet previous = s0;
  input->seek(startIndex);
  size_t t = input->LA(1);
  size_t predictedAlt;

  while (true) {
    reach = computeReachSet(previous, t, fullCtx);
    if (reach.isEmpty()) {
      // if any configs in previous dipped into outer context, that
      // means that input up to t actually finished entry rule
      // at least for LL decision. Full LL doesn't dip into outer
      // so don't need special case.
      // We will get an error no matter what so delay until after
      // decision; better error message. Also, no reachable target
      // ATN states in SLL implies LL will also get nowhere.
      // If conflict in states that dip out, choose min since we
      // will get error no matter what.
      input->seek(startIndex);
      size_t alt = getSynValidOrSemInvalidAltThatFinishedDecisionEntryRule(previous, outerContext);
      if (alt != ATN::INVALID_ALT_NUMBER) {
        return alt;
      }
      throw noViableAlt(input, outerContext, previous, startIndex);
    }
    previous = ATNConfigSet();

    std::vector<BitSet> altSubSets = PredictionModeClass::getConflictingAltSubsets(reach);
    reach.uniqueAlt = getUniqueAlt(reach);
    // unique prediction?
    if (reach.uniqueAlt != ATN::INVALID_ALT_NUMBER) {
      predictedAlt = reach.uniqueAlt;
      break;
    }
    if (_mode != PredictionMode::LL_EXACT_AMBIG_DETECTION) {
      predictedAlt = PredictionModeClass::resolvesToJustOneViableAlt(altSubSets);
      if (predictedAlt != ATN::INVALID_ALT_NUMBER) {
        break;
      }
    } else {
      // In exact ambiguity mode, we never try to terminate early.
      // Just keeps scarfing until we know what the conflict is
      if (PredictionModeClass::allSubsetsConflict(altSubSets) && PredictionModeClass::allSubsetsEqual(altSubSets)) {
        foundExactAmbig = true;
        predictedAlt = PredictionModeClass::getSingleViableAlt(altSubSets);
        break;
      }
      // else there are multiple non-conflicting subsets or
      // we're not sure what the ambiguity is yet.
      // So, keep going.
    }
    previous = reach;

    if (t != Token::EOF) {
      input->consume();
      t = input->LA(1);
    }
  }

  // If the configuration set uniquely predicts an alternative,
  // without conflict, then we know that it's a full LL decision
  // not SLL.
  if (reach.uniqueAlt != ATN::INVALID_ALT_NUMBER) {
    reportContextSensitivity(dfa, predictedAlt, reach, startIndex, input->index());
    return predictedAlt;
  }

  // We do not check predicates here because we have checked them
  // on-the-fly when doing full context prediction.

  /*
   In non-exact ambiguity detection mode, we might	actually be able to
   detect an exact ambiguity, but I'm not going to spend the cycles
   needed to check. We only emit ambiguity warnings in exact ambiguity
   mode.

   For example, we might know that we have conflicting configurations.
   But, that does not mean that there is no way forward without a
   conflict. It's possible to have nonconflicting alt subsets as in:

   LL altSubSets=[{1, 2}, {1, 2}, {1}, {1, 2}]

   from

   [(17,1,[5 $]), (13,1,[5 10 $]), (21,1,[5 10 $]), (11,1,[$]),
   (13,2,[5 10 $]), (21,2,[5 10 $]), (11,2,[$])]

   In this case, (17,1,[5 $]) indicates there is some next sequence that
   would resolve this without conflict to alternative 1. Any other viable
   next sequence, however, is associated with a conflict.  We stop
   looking for input because no amount of further lookahead will alter
   the fact that we should predict alternative 1.  We just can't say for
   sure that there is an ambiguity without looking further.
   */
  reportAmbiguity(dfa, D, startIndex, input->index(), foundExactAmbig, reach.getAlts(), reach);

  return predictedAlt;
}

ATNConfigSet ParserATNSimulator::computeReachSet(const ATNConfigSet &closure_, size_t t, bool fullCtx) {

  ATNConfigSet intermediate(fullCtx);

  /* Configurations already in a rule stop state indicate reaching the end
   * of the decision rule (local context) or end of the start rule (full
   * context). Once reached, these configurations are never updated by a
   * closure operation, so they are handled separately for the performance
   * advantage of having a smaller intermediate set when calling closure.
   *
   * For full-context reach operations, separate handling is required to
   * ensure that the alternative matching the longest overall sequence is
   * chosen when multiple such configurations can match the input.
   */
  std::vector<const ATNConfig*> skippedStopStates;

  // First figure out where we can reach on input t
  for (const auto &c : closure_) {
    if (c.state->getStateType() == ATNState::RULE_STOP) {
      assert(c.context.isEmpty());

      if (fullCtx || t == Token::EOF) {
        skippedStopStates.push_back(&c);
      }

      continue;
    }

    size_t n = c.state->transitions.size();
    for (size_t ti = 0; ti < n; ti++) { // for each transition
      const AnyTransition &trans = c.state->transitions[ti];
      ATNState *target = getReachableTarget(trans, (int)t);
      if (target != nullptr) {
        intermediate.add(ATNConfig(c, target));
      }
    }
  }

  // Now figure out where the reach operation can take us...
  ATNConfigSet reach;

  /* This block optimizes the reach operation for intermediate sets which
   * trivially indicate a termination state for the overall
   * adaptivePredict operation.
   *
   * The conditions assume that intermediate
   * contains all configurations relevant to the reach set, but this
   * condition is not true when one or more configurations have been
   * withheld in skippedStopStates, or when the current symbol is EOF.
   */
  if (skippedStopStates.empty() && t != Token::EOF) {
    if (intermediate.size() == 1) {
      // Don't pursue the closure if there is just one state.
      // It can only have one alternative; just add to result
      // Also don't pursue the closure if there is unique alternative
      // among the configurations.
      reach = std::move(intermediate);
    } else if (getUniqueAlt(intermediate) != ATN::INVALID_ALT_NUMBER) {
      // Also don't pursue the closure if there is unique alternative
      // among the configurations.
      reach = std::move(intermediate);
    }
  }

  /* If the reach set could not be trivially determined, perform a closure
   * operation on the intermediate set to compute its initial value.
   */
  if (reach.isEmpty()) {
    std::unordered_set<ATNConfig> closureBusy;

    bool treatEofAsEpsilon = t == Token::EOF;
    for (const auto &c : intermediate) {
      closure(c, &reach, closureBusy, false, fullCtx, treatEofAsEpsilon);
    }
  }

  if (t == IntStream::EOF) {
    /* After consuming EOF no additional input is possible, so we are
     * only interested in configurations which reached the end of the
     * decision rule (local context) or end of the start rule (full
     * context). Update reach to contain only these configurations. This
     * handles both explicit EOF transitions in the grammar and implicit
     * EOF transitions following the end of the decision or start rule.
     *
     * When reach==intermediate, no closure operation was performed. In
     * this case, removeAllConfigsNotInRuleStopState needs to check for
     * reachable rule stop states as well as configurations already in
     * a rule stop state.
     *
     * This is handled before the configurations in skippedStopStates,
     * because any configurations potentially added from that list are
     * already guaranteed to meet this condition whether or not it's
     * required.
     */
    reach = removeAllConfigsNotInRuleStopState(reach, reach == intermediate);
  }

  /* If skippedStopStates is not null, then it contains at least one
   * configuration. For full-context reach operations, these
   * configurations reached the end of the start rule, in which case we
   * only add them back to reach if no configuration during the current
   * closure operation reached such a state. This ensures adaptivePredict
   * chooses an alternative matching the longest overall sequence when
   * multiple alternatives are viable.
   */
  if (skippedStopStates.size() > 0 && (!fullCtx || !PredictionModeClass::hasConfigInRuleStopState(reach))) {
    assert(!skippedStopStates.empty());

    for (const auto *c : skippedStopStates) {
      reach.add(*c);
    }
  }
  return reach;
}

ATNConfigSet ParserATNSimulator::removeAllConfigsNotInRuleStopState(const ATNConfigSet &configs,
  bool lookToEndOfRule) {
  if (PredictionModeClass::allConfigsInRuleStopStates(configs)) {
    return configs;
  }

  ATNConfigSet result(configs.fullCtx); /* mem-check: released by caller */

  for (const auto &config : configs) {
    if (config.state->getStateType() == ATNState::RULE_STOP) {
      result.add(config);
      continue;
    }

    if (lookToEndOfRule && config.state->epsilonOnlyTransitions) {
      const misc::IntervalSet &nextTokens = atn.nextTokens(config.state);
      if (nextTokens.contains(Token::EPSILON)) {
        ATNState *endOfRuleState = atn.ruleToStopState[config.state->ruleIndex];
        result.add(ATNConfig(config, endOfRuleState));
      }
    }
  }

  return result;
}

ATNConfigSet ParserATNSimulator::computeStartState(ATNState *p, RuleContext *ctx, bool fullCtx) {
  // always at least the implicit call to start rule
  AnyPredictionContext initialContext = PredictionContext::fromRuleContext(atn, ctx);
  ATNConfigSet configs(fullCtx);

  for (size_t i = 0; i < p->transitions.size(); i++) {
    ATNState *target = p->transitions[i].getTarget();
    ATNConfig c = ATNConfig(target, (int)i + 1, initialContext);
    std::unordered_set<ATNConfig> closureBusy;
    closure(c, &configs, closureBusy, true, fullCtx, false);
  }

  return configs;
}

atn::ATNState* ParserATNSimulator::getReachableTarget(const Transition &trans, size_t ttype) {
  if (trans.matches(ttype, 0, atn.maxTokenType)) {
    return trans.getTarget();
  }

  return nullptr;
}

// Note that caller must memory manage the returned value from this function
std::vector<AnySemanticContext> ParserATNSimulator::getPredsForAmbigAlts(const BitSet &ambigAlts,
  const ATNConfigSet &configs, size_t nalts) {
  // REACH=[1|1|[]|0:0, 1|2|[]|0:1]
  /* altToPred starts as an array of all null contexts. The entry at index i
   * corresponds to alternative i. altToPred[i] may have one of three values:
   *   1. null: no ATNConfig c is found such that c.alt==i
   *   2. SemanticContext.NONE: At least one ATNConfig c exists such that
   *      c.alt==i and c.semanticContext==SemanticContext.NONE. In other words,
   *      alt i has at least one un-predicated config.
   *   3. Non-NONE Semantic Context: There exists at least one, and for all
   *      ATNConfig c such that c.alt==i, c.semanticContext!=SemanticContext.NONE.
   *
   * From this, it is clear that NONE||anything==NONE.
   */
  std::vector<AnySemanticContext> altToPred(nalts + 1);

  for (const auto &c : configs) {
    if (ambigAlts.test(c.alt)) {
      altToPred[c.alt] = SemanticContext::Or(altToPred[c.alt], c.semanticContext);
    }
  }

  size_t nPredAlts = 0;
  for (size_t i = 1; i <= nalts; i++) {
    if (!altToPred[i].valid()) {
      altToPred[i] = SemanticContext::none();
    } else if (altToPred[i] != SemanticContext::none()) {
      nPredAlts++;
    }
  }

  // nonambig alts are null in altToPred
  if (nPredAlts == 0) {
    altToPred.clear();
  }
#if DEBUG_ATN == 1
    std::cout << "getPredsForAmbigAlts result " << Arrays::toString(altToPred) << std::endl;
#endif

  return altToPred;
}

std::vector<dfa::DFAState::PredPrediction> ParserATNSimulator::getPredicatePredictions(const antlrcpp::BitSet &ambigAlts,
  const std::vector<AnySemanticContext> &altToPred) {
  bool containsPredicate = std::find(altToPred.begin(), altToPred.end(), SemanticContext::none()) != altToPred.end();
  if (!containsPredicate) {
    return {};
  }

  std::vector<dfa::DFAState::PredPrediction> pairs;
  for (size_t i = 1; i < altToPred.size(); ++i) {
    const AnySemanticContext &pred = altToPred[i];
    assert(pred.valid()); // unpredicted is indicated by SemanticContext.NONE

    if (ambigAlts.test(i)) {
      pairs.emplace_back(pred, static_cast<int>(i));
    }
  }
  pairs.shrink_to_fit();
  return pairs;
}

size_t ParserATNSimulator::getSynValidOrSemInvalidAltThatFinishedDecisionEntryRule(const ATNConfigSet &configs,
  ParserRuleContext *outerContext)
{
  std::pair<ATNConfigSet, ATNConfigSet> sets = splitAccordingToSemanticValidity(configs, outerContext);
  size_t alt = getAltThatFinishedDecisionEntryRule(sets.first);
  if (alt != ATN::INVALID_ALT_NUMBER) { // semantically/syntactically viable path exists
    return alt;
  }
  // Is there a syntactically valid path with a failed pred?
  if (!sets.second.isEmpty()) {
    alt = getAltThatFinishedDecisionEntryRule(sets.second);
    if (alt != ATN::INVALID_ALT_NUMBER) { // syntactically viable path exists
      return alt;
    }
  }
  return ATN::INVALID_ALT_NUMBER;
}

size_t ParserATNSimulator::getAltThatFinishedDecisionEntryRule(const ATNConfigSet &configs) {
  misc::IntervalSet alts;
  for (const auto &c : configs) {
    if (c.getOuterContextDepth() > 0 || (c.state->getStateType() == ATNState::RULE_STOP && c.context.hasEmptyPath())) {
      alts.add(c.alt);
    }
  }
  if (alts.size() == 0) {
    return ATN::INVALID_ALT_NUMBER;
  }
  return alts.getMinElement();
}

std::pair<ATNConfigSet, ATNConfigSet> ParserATNSimulator::splitAccordingToSemanticValidity(const ATNConfigSet &configs,
  ParserRuleContext *outerContext) {

  // mem-check: both pointers must be freed by the caller.
  ATNConfigSet succeeded(configs.fullCtx);
  ATNConfigSet failed(configs.fullCtx);
  for (const auto &c : configs) {
    if (c.semanticContext != SemanticContext::none()) {
      bool predicateEvaluationResult = evalSemanticContext(c.semanticContext, outerContext, c.alt, configs.fullCtx);
      if (predicateEvaluationResult) {
        succeeded.add(c);
      } else {
        failed.add(c);
      }
    } else {
      succeeded.add(c);
    }
  }
  return { succeeded, failed };
}

BitSet ParserATNSimulator::evalSemanticContext(const std::vector<dfa::DFAState::PredPrediction> &predPredictions,
                                               ParserRuleContext *outerContext, bool complete) {
  BitSet predictions;
  for (const auto &prediction : predPredictions) {
    if (prediction.pred == SemanticContext::none()) {
      predictions.set(prediction.alt);
      if (!complete) {
        break;
      }
      continue;
    }

    bool fullCtx = false; // in dfa
    bool predicateEvaluationResult = evalSemanticContext(prediction.pred, outerContext, prediction.alt, fullCtx);
#if DEBUG_ATN == 1 || DEBUG_DFA == 1
      std::cout << "eval pred " << prediction->toString() << " = " << predicateEvaluationResult << std::endl;
#endif

    if (predicateEvaluationResult) {
#if DEBUG_ATN == 1 || DEBUG_DFA == 1
        std::cout << "PREDICT " << prediction->alt << std::endl;
#endif

      predictions.set(prediction.alt);
      if (!complete) {
        break;
      }
    }
  }

  return predictions;
}

bool ParserATNSimulator::evalSemanticContext(const AnySemanticContext &pred, ParserRuleContext *parserCallStack,
                                             size_t /*alt*/, bool /*fullCtx*/) {
  return pred.eval(parser, parserCallStack);
}

void ParserATNSimulator::closure(const ATNConfig &config, ATNConfigSet *configs, std::unordered_set<ATNConfig> &closureBusy,
                                 bool collectPredicates, bool fullCtx, bool treatEofAsEpsilon) {
  const int initialDepth = 0;
  closureCheckingStopState(config, configs, closureBusy, collectPredicates, fullCtx, initialDepth, treatEofAsEpsilon);

  assert(!fullCtx || !configs->dipsIntoOuterContext);
}

void ParserATNSimulator::closureCheckingStopState(const ATNConfig &config, ATNConfigSet *configs,
  std::unordered_set<ATNConfig> &closureBusy, bool collectPredicates, bool fullCtx, int depth, bool treatEofAsEpsilon) {

#if DEBUG_ATN == 1
    std::cout << "closure(" << config->toString(true) << ")" << std::endl;
#endif

  if (config.state->getStateType() == ATNState::RULE_STOP) {
    // We hit rule end. If we have context info, use it
    // run thru all possible stack tops in ctx
    if (!config.context.isEmpty()) {
      for (size_t i = 0; i < config.context.size(); i++) {
        if (config.context.getReturnState(i) == PredictionContext::EMPTY_RETURN_STATE) {
          if (fullCtx) {
            configs->add(ATNConfig(config, config.state, PredictionContext::empty()));
            continue;
          } else {
            // we have no context info, just chase follow links (if greedy)
#if DEBUG_ATN == 1
              std::cout << "FALLING off rule " << getRuleName(config.state->ruleIndex) << std::endl;
#endif
            closure(config, configs, closureBusy, collectPredicates, fullCtx, depth, treatEofAsEpsilon);
          }
          continue;
        }
        ATNState *returnState = atn.states[config.context.getReturnState(i)].get();
        AnyPredictionContext newContext = config.context.getParent(i); // "pop" return state
        ATNConfig c = ATNConfig(returnState, config.alt, std::move(newContext), config.semanticContext);
        // While we have context to pop back from, we may have
        // gotten that context AFTER having falling off a rule.
        // Make sure we track that we are now out of context.
        //
        // This assignment also propagates the
        // isPrecedenceFilterSuppressed() value to the new
        // configuration.
        c.reachesIntoOuterContext = config.reachesIntoOuterContext;
        assert(depth > INT_MIN);

        closureCheckingStopState(c, configs, closureBusy, collectPredicates, fullCtx, depth - 1, treatEofAsEpsilon);
      }
      return;
    } else if (fullCtx) {
      // reached end of start rule
      configs->add(config);
      return;
    } else {
      // else if we have no context info, just chase follow links (if greedy)
    }
  }

  closure(config, configs, closureBusy, collectPredicates, fullCtx, depth, treatEofAsEpsilon);
}

void ParserATNSimulator::closure(const ATNConfig &config, ATNConfigSet *configs, std::unordered_set<ATNConfig> &closureBusy,
                                bool collectPredicates, bool fullCtx, int depth, bool treatEofAsEpsilon) {
  ATNState *p = config.state;
  // optimization
  if (!p->epsilonOnlyTransitions) {
    // make sure to not return here, because EOF transitions can act as
    // both epsilon transitions and non-epsilon transitions.
    configs->add(config);
  }

  for (size_t i = 0; i < p->transitions.size(); i++) {
    if (i == 0 && canDropLoopEntryEdgeInLeftRecursiveRule(config))
      continue;

    const AnyTransition &t = p->transitions[i];
    bool continueCollecting = !t.is<ActionTransition>() && collectPredicates;
    std::optional<ATNConfig> c = getEpsilonTarget(config, t, continueCollecting, depth == 0, fullCtx, treatEofAsEpsilon);
    if (c.has_value()) {
      int newDepth = depth;
      if (config.state->getStateType() == ATNState::RULE_STOP) {
        assert(!fullCtx);

        // target fell off end of rule; mark resulting c as having dipped into outer context
        // We can't get here if incoming config was rule stop and we had context
        // track how far we dip into outer context.  Might
        // come in handy and we avoid evaluating context dependent
        // preds if this is > 0.

        if (closureBusy.count(c.value()) > 0) {
          // avoid infinite recursion for right-recursive rules
          continue;
        }

        if (_dfa != nullptr && _dfa->isPrecedenceDfa()) {
          size_t outermostPrecedenceReturn = t.as<EpsilonTransition>().getOutermostPrecedenceReturn();
          if (outermostPrecedenceReturn == _dfa->atnStartState->ruleIndex) {
            c.value().setPrecedenceFilterSuppressed(true);
          }
        }

        c.value().reachesIntoOuterContext++;

        // avoid infinite recursion for EOF* and EOF+
        auto [_, inserted] = closureBusy.insert(c.value());
        if (!inserted) {
          continue;
        }

        configs->dipsIntoOuterContext = true; // TODO: can remove? only care when we add to set per middle of this method
        assert(newDepth > INT_MIN);

        newDepth--;
#if DEBUG_DFA == 1
          std::cout << "dips into outer ctx: " << c << std::endl;
#endif

      } else  if (!t.isEpsilon()) {
        // avoid infinite recursion for EOF* and EOF+
        auto [_, inserted] = closureBusy.insert(c.value());
        if (!inserted) {
          continue;
        }
      }

      if (t.is<RuleTransition>()) {
        // latch when newDepth goes negative - once we step out of the entry context we can't return
        if (newDepth >= 0) {
          newDepth++;
        }
      }

      closureCheckingStopState(c.value(), configs, closureBusy, continueCollecting, fullCtx, newDepth, treatEofAsEpsilon);
    }
  }
}

bool ParserATNSimulator::canDropLoopEntryEdgeInLeftRecursiveRule(const ATNConfig &config) const {
  if (TURN_OFF_LR_LOOP_ENTRY_BRANCH_OPT)
    return false;

  ATNState *p = config.state;

  // First check to see if we are in StarLoopEntryState generated during
  // left-recursion elimination. For efficiency, also check if
  // the context has an empty stack case. If so, it would mean
  // global FOLLOW so we can't perform optimization
  if (p->getStateType() != ATNState::STAR_LOOP_ENTRY ||
      !((StarLoopEntryState *)p)->isPrecedenceDecision || // Are we the special loop entry/exit state?
      config.context.isEmpty() ||                      // If SLL wildcard
      config.context.hasEmptyPath())
  {
    return false;
  }

  // Require all return states to return back to the same rule
  // that p is in.
  size_t numCtxs = config.context.size();
  for (size_t i = 0; i < numCtxs; i++) { // for each stack context
    ATNState *returnState = atn.states[config.context.getReturnState(i)].get();
    if (returnState->ruleIndex != p->ruleIndex)
      return false;
  }

  BlockStartState *decisionStartState = (BlockStartState *)p->transitions[0].getTarget();
  size_t blockEndStateNum = decisionStartState->endState->stateNumber;
  BlockEndState *blockEndState = (BlockEndState *)atn.states[blockEndStateNum].get();

  // Verify that the top of each stack context leads to loop entry/exit
  // state through epsilon edges and w/o leaving rule.
  for (size_t i = 0; i < numCtxs; i++) {                           // for each stack context
    size_t returnStateNumber = config.context.getReturnState(i);
    ATNState *returnState = atn.states[returnStateNumber].get();
    // All states must have single outgoing epsilon edge.
    if (returnState->transitions.size() != 1 || !returnState->transitions[0].isEpsilon())
    {
      return false;
    }

    // Look for prefix op case like 'not expr', (' type ')' expr
    ATNState *returnStateTarget = returnState->transitions[0].getTarget();
    if (returnState->getStateType() == ATNState::BLOCK_END && returnStateTarget == p) {
      continue;
    }

    // Look for 'expr op expr' or case where expr's return state is block end
    // of (...)* internal block; the block end points to loop back
    // which points to p but we don't need to check that
    if (returnState == blockEndState) {
      continue;
    }

    // Look for ternary expr ? expr : expr. The return state points at block end,
    // which points at loop entry state
    if (returnStateTarget == blockEndState) {
      continue;
    }

    // Look for complex prefix 'between expr and expr' case where 2nd expr's
    // return state points at block end state of (...)* internal block
    if (returnStateTarget->getStateType() == ATNState::BLOCK_END &&
        returnStateTarget->transitions.size() == 1 &&
        returnStateTarget->transitions[0].isEpsilon() &&
        returnStateTarget->transitions[0].getTarget() == p)
    {
      continue;
    }

    // Anything else ain't conforming.
    return false;
  }

  return true;
}

std::string ParserATNSimulator::getRuleName(size_t index) {
  if (parser != nullptr) {
    return parser->getRuleNames()[index];
  }
  return "<rule " + std::to_string(index) + ">";
}

std::optional<ATNConfig> ParserATNSimulator::getEpsilonTarget(const ATNConfig &config, const Transition &t, bool collectPredicates,
                                                    bool inContext, bool fullCtx, bool treatEofAsEpsilon) {
  switch (t.getType()) {
    case TransitionType::RULE:
      return ruleTransition(config, downCast<const RuleTransition&>(t));

    case TransitionType::PRECEDENCE:
      return precedenceTransition(config, downCast<const PrecedencePredicateTransition&>(t), collectPredicates, inContext, fullCtx);

    case TransitionType::PREDICATE:
      return predTransition(config, downCast<const PredicateTransition&>(t), collectPredicates, inContext, fullCtx);

    case TransitionType::ACTION:
      return actionTransition(config, downCast<const ActionTransition&>(t));

    case TransitionType::EPSILON:
      return ATNConfig(config, t.getTarget());

    case TransitionType::ATOM:
    case TransitionType::RANGE:
    case TransitionType::SET:
      // EOF transitions act like epsilon transitions after the first EOF
      // transition is traversed
      if (treatEofAsEpsilon) {
        if (t.matches(Token::EOF, 0, 1)) {
          return ATNConfig(config, t.getTarget());
        }
      }

      return std::nullopt;

    default:
      return std::nullopt;
  }
}

ATNConfig ParserATNSimulator::actionTransition(const ATNConfig &config, const ActionTransition &t) {
#if DEBUG_DFA == 1
    std::cout << "ACTION edge " << t.getRuleIndex() << ":" << t.getActionIndex() << std::endl;
#endif

  return ATNConfig(config, t.getTarget());
}

ATNConfig ParserATNSimulator::precedenceTransition(const ATNConfig &config, const PrecedencePredicateTransition &pt,
    bool collectPredicates, bool inContext, bool fullCtx) {
#if DEBUG_DFA == 1
    std::cout << "PRED (collectPredicates=" << collectPredicates << ") " << pt.getPrecedence() << ">=_p" << ", ctx dependent=true" << std::endl;
    if (parser != nullptr) {
      std::cout << "context surrounding pred is " << Arrays::listToString(parser->getRuleInvocationStack(), ", ") << std::endl;
    }
#endif

  ATNConfig c;
  if (collectPredicates && inContext) {
    SemanticContext::PrecedencePredicate predicate = pt.getPredicate();

    if (fullCtx) {
      // In full context mode, we can evaluate predicates on-the-fly
      // during closure, which dramatically reduces the size of
      // the config sets. It also obviates the need to test predicates
      // later during conflict resolution.
      size_t currentPosition = _input->index();
      _input->seek(_startIndex);
      bool predSucceeds = evalSemanticContext(predicate, _outerContext, config.alt, fullCtx);
      _input->seek(currentPosition);
      if (predSucceeds) {
        c = ATNConfig(config, pt.getTarget()); // no pred context
      }
    } else {
      AnySemanticContext newSemCtx = SemanticContext::And(config.semanticContext, predicate);
      c = ATNConfig(config, pt.getTarget(), std::move(newSemCtx));
    }
  } else {
    c = ATNConfig(config, pt.getTarget());
  }

#if DEBUG_DFA == 1
    std::cout << "config from pred transition=" << c << std::endl;
#endif

  return c;
}

ATNConfig ParserATNSimulator::predTransition(const ATNConfig &config, const PredicateTransition &pt,
  bool collectPredicates, bool inContext, bool fullCtx) {
#if DEBUG_DFA == 1
    std::cout << "PRED (collectPredicates=" << collectPredicates << ") " << pt.getRuleIndex() << ":" << pt.getPredIndex() << ", ctx dependent=" << pt.isCtxDependent() << std::endl;
    if (parser != nullptr) {
      std::cout << "context surrounding pred is " << Arrays::listToString(parser->getRuleInvocationStack(), ", ") << std::endl;
    }
#endif

  ATNConfig c;
  if (collectPredicates && (!pt.isCtxDependent() || (pt.isCtxDependent() && inContext))) {
    SemanticContext::Predicate predicate = pt.getPredicate();
    if (fullCtx) {
      // In full context mode, we can evaluate predicates on-the-fly
      // during closure, which dramatically reduces the size of
      // the config sets. It also obviates the need to test predicates
      // later during conflict resolution.
      size_t currentPosition = _input->index();
      _input->seek(_startIndex);
      bool predSucceeds = evalSemanticContext(predicate, _outerContext, config.alt, fullCtx);
      _input->seek(currentPosition);
      if (predSucceeds) {
        c = ATNConfig(config, pt.getTarget()); // no pred context
      }
    } else {
      AnySemanticContext newSemCtx = SemanticContext::And(config.semanticContext, predicate);
      c = ATNConfig(config, pt.getTarget(), std::move(newSemCtx));
    }
  } else {
    c = ATNConfig(config, pt.getTarget());
  }

#if DEBUG_DFA == 1
    std::cout << "config from pred transition=" << c << std::endl;
#endif

  return c;
}

ATNConfig ParserATNSimulator::ruleTransition(const ATNConfig &config, const RuleTransition &t) {
#if DEBUG_DFA == 1
    std::cout << "CALL rule " << getRuleName(t.getTarget()->ruleIndex) << ", ctx=" << config->context << std::endl;
#endif

  atn::ATNState *returnState = t.getFollowState();
  AnyPredictionContext newContext = SingletonPredictionContext::create(config.context, returnState->stateNumber);
  return ATNConfig(config, t.getTarget(), std::move(newContext));
}

BitSet ParserATNSimulator::getConflictingAlts(const ATNConfigSet &configs) {
  std::vector<BitSet> altsets = PredictionModeClass::getConflictingAltSubsets(configs);
  return PredictionModeClass::getAlts(altsets);
}

BitSet ParserATNSimulator::getConflictingAltsOrUniqueAlt(const ATNConfigSet &configs) const {
  BitSet conflictingAlts;
  if (configs.uniqueAlt != ATN::INVALID_ALT_NUMBER) {
    conflictingAlts.set(configs.uniqueAlt);
  } else {
    conflictingAlts = configs.conflictingAlts;
  }
  return conflictingAlts;
}

std::string ParserATNSimulator::getTokenName(size_t t) {
  if (t == Token::EOF) {
    return "EOF";
  }

  const dfa::Vocabulary &vocabulary = parser != nullptr ? parser->getVocabulary() : dfa::Vocabulary();
  std::string displayName = vocabulary.getDisplayName(t);
  if (displayName == std::to_string(t)) {
    return displayName;
  }

  return displayName + "<" + std::to_string(t) + ">";
}

std::string ParserATNSimulator::getLookaheadName(TokenStream *input) {
  return getTokenName(input->LA(1));
}

void ParserATNSimulator::dumpDeadEndConfigs(NoViableAltException &nvae) {
  std::cerr << "dead end configs: ";
  for (const auto &c : *nvae.getDeadEndConfigs()) {
    std::string trans = "no edges";
    if (c.state->transitions.size() > 0) {
      const AnyTransition &t = c.state->transitions[0];
      if (t.is<AtomTransition>()) {
        trans = "Atom " + getTokenName(static_cast<size_t>(t.label().getSingleElement()));
      } else if (t.is<SetTransition>()) {
        trans += "Set ";
        trans += t.label().toString();
      } else if (t.is<NotSetTransition>()) {
        trans += "~Set ";
        trans += t.label().toString();
      }
    }
    std::cerr << c.toString(true) + ":" + trans;
  }
}

NoViableAltException ParserATNSimulator::noViableAlt(TokenStream *input, ParserRuleContext *outerContext,
  const ATNConfigSet &configs, size_t startIndex) {
  return NoViableAltException(parser, input, input->get(startIndex), input->LT(1), configs, outerContext);
}

size_t ParserATNSimulator::getUniqueAlt(const ATNConfigSet &configs) {
  size_t alt = ATN::INVALID_ALT_NUMBER;
  for (const auto &c : configs) {
    if (alt == ATN::INVALID_ALT_NUMBER) {
      alt = c.alt; // found first alt
    } else if (c.alt != alt) {
      return ATN::INVALID_ALT_NUMBER;
    }
  }
  return alt;
}

dfa::DFAState *ParserATNSimulator::addDFAEdge(dfa::DFA &dfa, dfa::DFAState *from, ssize_t t, std::unique_ptr<dfa::DFAState> to) {
#if DEBUG_DFA == 1
    std::cout << "EDGE " << from << " -> " << to << " upon " << getTokenName(t) << std::endl;
#endif

  if (to == nullptr) {
    return nullptr;
  }

  auto *s0 = atn.addParserDFAState(dfa, std::move(to));
  if (from == nullptr || t > (int)atn.maxTokenType) {
    return s0;
  }

  atn.addParserDFAEdge(from, t, s0);

#if DEBUG_DFA == 1
    std::string dfaText;
    if (parser != nullptr) {
      dfaText = dfa.toString(parser->getVocabulary());
    } else {
      dfaText = dfa.toString(dfa::Vocabulary());
    }
    std::cout << "DFA=\n" << dfaText << std::endl;
#endif

  return s0;
}

void ParserATNSimulator::reportAttemptingFullContext(dfa::DFA &dfa, const antlrcpp::BitSet &conflictingAlts,
  const ATNConfigSet &configs, size_t startIndex, size_t stopIndex) {
#if DEBUG_DFA == 1 || RETRY_DEBUG == 1
    misc::Interval interval = misc::Interval((int)startIndex, (int)stopIndex);
    std::cout << "reportAttemptingFullContext decision=" << dfa.decision << ":" << configs << ", input=" << parser->getTokenStream()->getText(interval) << std::endl;
#endif

  if (parser != nullptr) {
    parser->getErrorListenerDispatch().reportAttemptingFullContext(parser, dfa, startIndex, stopIndex, conflictingAlts, configs);
  }
}

void ParserATNSimulator::reportContextSensitivity(dfa::DFA &dfa, size_t prediction, const ATNConfigSet &configs,
  size_t startIndex, size_t stopIndex) {
#if DEBUG_DFA == 1 || RETRY_DEBUG == 1
    misc::Interval interval = misc::Interval(startIndex, stopIndex);
    std::cout << "reportContextSensitivity decision=" << dfa.decision << ":" << configs << ", input=" << parser->getTokenStream()->getText(interval) << std::endl;
#endif

  if (parser != nullptr) {
    parser->getErrorListenerDispatch().reportContextSensitivity(parser, dfa, startIndex, stopIndex, prediction, configs);
  }
}

void ParserATNSimulator::reportAmbiguity(dfa::DFA &dfa, dfa::DFAState * /*D*/, size_t startIndex, size_t stopIndex,
                                         bool exact, const antlrcpp::BitSet &ambigAlts, const ATNConfigSet &configs) {
#if DEBUG_DFA == 1 || RETRY_DEBUG == 1
    misc::Interval interval = misc::Interval((int)startIndex, (int)stopIndex);
    std::cout << "reportAmbiguity " << ambigAlts << ":" << configs << ", input=" << parser->getTokenStream()->getText(interval) << std::endl;
#endif

  if (parser != nullptr) {
    parser->getErrorListenerDispatch().reportAmbiguity(parser, dfa, startIndex, stopIndex, exact, ambigAlts, configs);
  }
}

void ParserATNSimulator::setPredictionMode(PredictionMode newMode) {
  _mode = newMode;
}

atn::PredictionMode ParserATNSimulator::getPredictionMode() {
  return _mode;
}

Parser* ParserATNSimulator::getParser() {
  return parser;
}

#ifdef _MSC_VER
#pragma warning (disable:4996) // 'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead.
#endif

bool ParserATNSimulator::getLrLoopSetting() {
  char *var = std::getenv("TURN_OFF_LR_LOOP_ENTRY_BRANCH_OPT");
  if (var == nullptr)
    return false;
  std::string value(var);
  return value == "true" || value == "1";
}
