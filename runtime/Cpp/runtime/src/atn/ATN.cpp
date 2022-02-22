/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "atn/LL1Analyzer.h"
#include "Token.h"
#include "atn/RuleTransition.h"
#include "Parser.h"
#include "misc/IntervalSet.h"
#include "RuleContext.h"
#include "atn/DecisionState.h"
#include "Recognizer.h"
#include "dfa/DFA.h"
#include "atn/AnySemanticContext.h"
#include "dfa/DFAState.h"
#include "atn/ATNType.h"
#include "atn/ParserATNSimulator.h"
#include "atn/LexerATNSimulator.h"
#include "Exceptions.h"
#include "support/CPPUtils.h"

#include "atn/ATN.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::dfa;
using namespace antlrcpp;

namespace {


/* parrt internal source braindump that doesn't mess up
  * external API spec.

  applyPrecedenceFilter is an optimization to avoid highly
  nonlinear prediction of expressions and other left recursive
  rules. The precedence predicates such as {3>=prec}? Are highly
  context-sensitive in that they can only be properly evaluated
  in the context of the proper prec argument. Without pruning,
  these predicates are normal predicates evaluated when we reach
  conflict state (or unique prediction). As we cannot evaluate
  these predicates out of context, the resulting conflict leads
  to full LL evaluation and nonlinear prediction which shows up
  very clearly with fairly large expressions.

  Example grammar:

  e : e '*' e
  | e '+' e
  | INT
  ;

  We convert that to the following:

  e[int prec]
  :   INT
  ( {3>=prec}? '*' e[4]
  | {2>=prec}? '+' e[3]
  )*
  ;

  The (..)* loop has a decision for the inner block as well as
  an enter or exit decision, which is what concerns us here. At
  the 1st + of input 1+2+3, the loop entry sees both predicates
  and the loop exit also sees both predicates by falling off the
  edge of e.  This is because we have no stack information with
  SLL and find the follow of e, which will hit the return states
  inside the loop after e[4] and e[3], which brings it back to
  the enter or exit decision. In this case, we know that we
  cannot evaluate those predicates because we have fallen off
  the edge of the stack and will in general not know which prec
  parameter is the right one to use in the predicate.

  Because we have special information, that these are precedence
  predicates, we can resolve them without failing over to full
  LL despite their context sensitive nature. We make an
  assumption that prec[-1] <= prec[0], meaning that the current
  precedence level is greater than or equal to the precedence
  level of recursive invocations above us in the stack. For
  example, if predicate {3>=prec}? is true of the current prec,
  then one option is to enter the loop to match it now. The
  other option is to exit the loop and the left recursive rule
  to match the current operator in rule invocation further up
  the stack. But, we know that all of those prec are lower or
  the same value and so we can decide to enter the loop instead
  of matching it later. That means we can strip out the other
  configuration for the exit branch.

  So imagine we have (14,1,$,{2>=prec}?) and then
  (14,2,$-dipsIntoOuterContext,{2>=prec}?). The optimization
  allows us to collapse these two configurations. We know that
  if {2>=prec}? is true for the current prec parameter, it will
  also be true for any prec from an invoking e call, indicated
  by dipsIntoOuterContext. As the predicates are both true, we
  have the option to evaluate them early in the decision start
  state. We do this by stripping both predicates and choosing to
  enter the loop as it is consistent with the notion of operator
  precedence. It's also how the full LL conflict resolution
  would work.

  The solution requires a different DFA start state for each
  precedence level.

  The basic filter mechanism is to remove configurations of the
  form (p, 2, pi) if (p, 1, pi) exists for the same p and pi. In
  other words, for the same ATN state and predicate context,
  remove any configuration associated with an exit branch if
  there is a configuration associated with the enter branch.

  It's also the case that the filter evaluates precedence
  predicates and resolves conflicts according to precedence
  levels. For example, for input 1+2+3 at the first +, we see
  prediction filtering

  [(11,1,[$],{3>=prec}?), (14,1,[$],{2>=prec}?), (5,2,[$],up=1),
  (11,2,[$],up=1), (14,2,[$],up=1)],hasSemanticContext=true,dipsIntoOuterContext

  to

  [(11,1,[$]), (14,1,[$]), (5,2,[$],up=1)],dipsIntoOuterContext

  This filters because {3>=prec}? evals to true and collapses
  (11,1,[$],{3>=prec}?) and (11,2,[$],up=1) since early conflict
  resolution based upon rules of operator precedence fits with
  our usual match first alt upon conflict.

  We noticed a problem where a recursive call resets precedence
  to 0. Sam's fix: each config has flag indicating if it has
  returned from an expr[0] call. then just don't filter any
  config with that flag set. flag is carried along in
  closure(). so to avoid adding field, set bit just under sign
  bit of dipsIntoOuterContext (SUPPRESS_PRECEDENCE_FILTER).
  With the change you filter "unless (p, 2, pi) was reached
  after leaving the rule stop state of the LR rule containing
  state p, corresponding to a rule invocation with precedence
  level 0"
  */

/**
 * This method transforms the start state computed by
 * {@link #computeStartState} to the special start state used by a
 * precedence DFA for a particular precedence value. The transformation
 * process applies the following changes to the start state's configuration
 * set.
 *
 * <ol>
 * <li>Evaluate the precedence predicates for each configuration using
 * {@link SemanticContext#evalPrecedence}.</li>
 * <li>When {@link ATNConfig#isPrecedenceFilterSuppressed} is {@code false},
 * remove all configurations which predict an alternative greater than 1,
 * for which another configuration that predicts alternative 1 is in the
 * same ATN state with the same prediction context. This transformation is
 * valid for the following reasons:
 * <ul>
 * <li>The closure block cannot contain any epsilon transitions which bypass
 * the body of the closure, so all states reachable via alternative 1 are
 * part of the precedence alternatives of the transformed left-recursive
 * rule.</li>
 * <li>The "primary" portion of a left recursive rule cannot contain an
 * epsilon transition, so the only way an alternative other than 1 can exist
 * in a state that is also reachable via alternative 1 is by nesting calls
 * to the left-recursive rule, with the outer calls not being at the
 * preferred precedence level. The
 * {@link ATNConfig#isPrecedenceFilterSuppressed} property marks ATN
 * configurations which do not meet this condition, and therefore are not
 * eligible for elimination during the filtering process.</li>
 * </ul>
 * </li>
 * </ol>
 *
 * <p>
 * The prediction context must be considered by this filter to address
 * situations like the following.
 * </p>
 * <code>
 * <pre>
 * grammar TA;
 * prog: statement* EOF;
 * statement: letterA | statement letterA 'b' ;
 * letterA: 'a';
 * </pre>
 * </code>
 * <p>
 * If the above grammar, the ATN state immediately before the token
 * reference {@code 'a'} in {@code letterA} is reachable from the left edge
 * of both the primary and closure blocks of the left-recursive rule
 * {@code statement}. The prediction context associated with each of these
 * configurations distinguishes between them, and prevents the alternative
 * which stepped out to {@code prog} (and then back in to {@code statement}
 * from being eliminated by the filter.
 * </p>
 *
 * @param configs The configuration set computed by
 * {@link #computeStartState} as the start state for the DFA.
 * @return The transformed configuration set representing the start state
 * for a precedence DFA at a particular precedence level (determined by
 * calling {@link Parser#getPrecedence}).
 */
ATNConfigSet applyPrecedenceFilter(const ATNConfigSet &configs, Parser* parser, RuleContext *context) {
  std::unordered_map<size_t, AnyPredictionContext> statesFromAlt1;
  ATNConfigSet configSet(configs.fullCtx);
  for (const auto &config : configs) {
    // handle alt 1 first
    if (config.alt != 1) {
      continue;
    }

    AnySemanticContext updatedContext = config.semanticContext.evalPrecedence(parser, context);
    if (!updatedContext) {
      // the configuration was eliminated
      continue;
    }

    statesFromAlt1[config.state->stateNumber] = config.context;
    if (updatedContext != config.semanticContext) {
      configSet.add(ATNConfig(config, updatedContext));
    }
    else {
      configSet.add(config);
    }
  }

  for (const auto &config : configs) {
    if (config.alt == 1) {
      // already handled
      continue;
    }

    if (!config.isPrecedenceFilterSuppressed()) {
      /* In the future, this elimination step could be updated to also
       * filter the prediction context for alternatives predicting alt>1
       * (basically a graph subtraction algorithm).
       */
      auto iterator = statesFromAlt1.find(config.state->stateNumber);
      if (iterator != statesFromAlt1.end() && iterator->second == config.context) {
        // eliminated
        continue;
      }
    }

    configSet.add(config);
  }

  return configSet;
}

DFAState* addParserDFAStateLocked(DFA &dfa, std::unique_ptr<dfa::DFAState> state) {
  if (state.get() == ATNSimulator::ERROR.get()) {
    return state.release();
  }
  // Optimizing the configs below should not alter the hash code. Thus we can just do an insert
  // which will only succeed if an equivalent DFAState does not already exist.
  auto [existing, inserted] = dfa.states.insert(std::move(state));
  if (inserted) {
    // Previously we did a lookup, then set fields, then inserted. It was `dfa.states.size()`, since
    // we already inserted we need to subtract one.
    existing->get()->stateNumber = static_cast<int>(dfa.states.size() - 1);
    if (!existing->get()->configs.isReadonly()) {
      existing->get()->configs.setReadonly(true);
    }
  }
  return existing->get();
}

}

ATN::ATN() : ATN(ATNType::LEXER, 0) {}

ATN::ATN(ATNType grammarType, size_t maxTokenType) : grammarType(grammarType), maxTokenType(maxTokenType) {}

misc::IntervalSet ATN::nextTokens(ATNState *s, RuleContext *ctx) const {
  LL1Analyzer analyzer(*this);
  return analyzer.LOOK(s, ctx);

}

misc::IntervalSet const& ATN::nextTokens(ATNState *s) const {
  if (!s->_nextTokenUpdated) {
    std::unique_lock<std::mutex> lock { _mutex };
    if (!s->_nextTokenUpdated) {
      s->_nextTokenWithinRule = nextTokens(s, nullptr);
      s->_nextTokenUpdated = true;
    }
  }
  return s->_nextTokenWithinRule;
}

int ATN::addState(std::unique_ptr<ATNState> state) {
  if (state != nullptr) {
    //state->atn = this;
    state->stateNumber = static_cast<int>(states.size());
  }
  states.push_back(std::move(state));
  return static_cast<int>(states.size() - 1);
}

void ATN::removeState(int stateNumber) {
  std::unique_ptr<ATNState> state;
  state.swap(states.at(static_cast<size_t>(stateNumber)));
}

int ATN::defineDecisionState(DecisionState *s) {
  decisionToState.push_back(s);
  s->decision = static_cast<int>(decisionToState.size() - 1);
  return s->decision;
}

DecisionState *ATN::getDecisionState(size_t decision) const {
  if (!decisionToState.empty()) {
    return decisionToState[decision];
  }
  return nullptr;
}

size_t ATN::getNumberOfDecisions() const {
  return decisionToState.size();
}

misc::IntervalSet ATN::getExpectedTokens(size_t stateNumber, RuleContext *context) const {
  if (stateNumber == ATNState::INVALID_STATE_NUMBER || stateNumber >= states.size()) {
    throw IllegalArgumentException("Invalid state number.");
  }

  RuleContext *ctx = context;
  ATNState *s = states.at(stateNumber).get();
  misc::IntervalSet following = nextTokens(s);
  if (!following.contains(Token::EPSILON)) {
    return following;
  }

  misc::IntervalSet expected;
  expected.addAll(following);
  expected.remove(Token::EPSILON);
  while (ctx && ctx->invokingState != ATNState::INVALID_STATE_NUMBER && following.contains(Token::EPSILON)) {
    ATNState *invokingState = states.at(ctx->invokingState).get();
    following = nextTokens(invokingState->transitions[0].as<RuleTransition>().getFollowState());
    expected.addAll(following);
    expected.remove(Token::EPSILON);

    if (ctx->parent == nullptr) {
      break;
    }
    ctx = static_cast<RuleContext *>(ctx->parent);
  }

  if (following.contains(Token::EPSILON)) {
    expected.add(Token::EOF);
  }

  return expected;
}

std::string ATN::toString() const {
  std::stringstream ss;
  std::string type;
  switch (grammarType) {
    case ATNType::LEXER:
      type = "LEXER ";
      break;

    case ATNType::PARSER:
      type = "PARSER ";
      break;

    default:
      break;
  }
  ss << "(" << type << "ATN " << std::hex << this << std::dec << ") maxTokenType: " << maxTokenType << std::endl;
  ss << "states (" << states.size() << ") {" << std::endl;

  size_t index = 0;
  for (auto &state : states) {
    if (state == nullptr) {
      ss << "  " << index++ << ": nul" << std::endl;
    } else {
      std::string text = state->toString();
      ss << "  " << index++ << ": " << indent(text, "  ", false) << std::endl;
    }
  }

  index = 0;
  for (auto *state : decisionToState) {
    if (state == nullptr) {
      ss << "  " << index++ << ": nul" << std::endl;
    } else {
      std::string text = state->toString();
      ss << "  " << index++ << ": " << indent(text, "  ", false) << std::endl;
    }
  }

  ss << "}";

  return ss.str();
}

DFAState* ATN::addParserDFAState(DFA &dfa, std::unique_ptr<dfa::DFAState> state) const {
  std::unique_lock<std::shared_mutex> stateLock(_stateMutex);
  return addParserDFAStateLocked(dfa, std::move(state));
}

DFAState* ATN::addLexerDFAState(DFA &dfa, std::unique_ptr<DFAState> state, bool suppressEdge) const {
  std::unique_lock<std::shared_mutex> stateLock(_stateMutex);
  auto [existing, inserted] = dfa.states.insert(std::move(state));
  if (!inserted) {
    if (!suppressEdge) {
      dfa.s0 = existing->get();
    }
    return existing->get();
  }
  // Previously we did a lookup, then set fields, then inserted. It was `dfa.states.size()`,
  // since we already inserted we need to subtract one.
  existing->get()->stateNumber = static_cast<int>(dfa.states.size() - 1);
  existing->get()->configs.setReadonly(true);
  if (!suppressEdge) {
    dfa.s0 = existing->get();
  }
  return existing->get();
}

DFAState* ATN::getParserStartState(const DFA &dfa, const Parser &parser) const {
  std::shared_lock<std::shared_mutex> stateLock(_stateMutex);
  if (dfa.isPrecedenceDfa()) {
    std::shared_lock<std::shared_mutex> edgeLock(_edgeMutex);
    auto existing = dfa.s0->edges.find(parser.getPrecedence());
    if (existing == dfa.s0->edges.end()) {
      return nullptr;
    }
    return existing->second;
  }
  // the start state for a "regular" DFA is just s0
  return dfa.s0;
}

DFAState* ATN::getLexerStartState(const DFA &dfa) const {
  std::shared_lock<std::shared_mutex> stateLock(_stateMutex);
  return dfa.s0;
}

DFAState* ATN::updateParserStartState(DFA &dfa, ATNConfigSet configs, Parser* parser, RuleContext *context) const {
  std::unique_lock<std::shared_mutex> stateLock(_stateMutex);
  DFAState *ds0 = dfa.s0;
  bool precedenceDfa = dfa.isPrecedenceDfa();
  DFAState *s0;
  if (precedenceDfa) {
    ds0->configs = std::move(configs);
    s0 = new DFAState(applyPrecedenceFilter(ds0->configs, parser, context));
  } else {
    s0 = new DFAState(std::move(configs));
  }
  s0 = addParserDFAStateLocked(dfa, std::unique_ptr<DFAState>(s0));
  if (precedenceDfa) {
    std::unique_lock<std::shared_mutex> edgeLock(_edgeMutex);
    int precedence = parser->getPrecedence();
    if (precedence >= 0) {
      dfa.s0->edges[precedence] = s0;
    }
  } else {
    if (ds0 != s0) {
      delete ds0; // Delete existing s0 DFA state, if there's any.
      ds0 = nullptr;
      dfa.s0 = s0;
    }
  }
  return s0;
}

DFAState* ATN::getParserExistingTargetState(const DFAState &state, size_t t) const {
  std::shared_lock<std::shared_mutex> edgeLock(_edgeMutex);
  auto existing = state.edges.find(t);
  if (existing == state.edges.end()) {
    return nullptr;
  }
  return existing->second;
}

DFAState* ATN::getLexerExistingTargetState(const DFAState &state, size_t t) const {
  if (t > LexerATNSimulator::MAX_DFA_EDGE) {
    return nullptr;
  }
  std::shared_lock<std::shared_mutex> edgeLock(_edgeMutex);
  auto existing = state.edges.find(t - LexerATNSimulator::MIN_DFA_EDGE);
  if (existing == state.edges.end()) {
    return nullptr;
  }
  return existing->second;
}

void ATN::addParserDFAEdge(DFAState *from, size_t t, DFAState *to) const {
  std::unique_lock<std::shared_mutex> edgeLock(_edgeMutex);
  from->edges[t] = to;
}

void ATN::addLexerDFAEdge(DFAState *from, size_t t, DFAState *to) const {
  if (/*t < MIN_DFA_EDGE ||*/ t > LexerATNSimulator::MAX_DFA_EDGE) { // MIN_DFA_EDGE is 0
    // Only track edges within the DFA bounds
    return;
  }
  std::unique_lock<std::shared_mutex> edgeLock(_edgeMutex);
  from->edges[t - LexerATNSimulator::MIN_DFA_EDGE] = to;
}

