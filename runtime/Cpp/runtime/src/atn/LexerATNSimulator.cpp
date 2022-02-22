/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "IntStream.h"
#include "atn/OrderedATNConfigSet.h"
#include "Token.h"
#include "LexerNoViableAltException.h"
#include "atn/RuleStopState.h"
#include "atn/RuleTransition.h"
#include "atn/AnyPredictionContext.h"
#include "atn/SingletonPredictionContext.h"
#include "atn/PredicateTransition.h"
#include "atn/ActionTransition.h"
#include "atn/AnyTransition.h"
#include "atn/TokensStartState.h"
#include "misc/Interval.h"
#include "dfa/DFA.h"
#include "Lexer.h"

#include "dfa/DFAState.h"
#include "atn/LexerATNConfig.h"
#include "atn/LexerActionExecutor.h"
#include "atn/EmptyPredictionContext.h"

#include "atn/LexerATNSimulator.h"

#define DEBUG_ATN 0
#define DEBUG_DFA 0

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlrcpp;

LexerATNSimulator::SimState::~SimState() {
}

void LexerATNSimulator::SimState::reset() {
  index = INVALID_INDEX;
  line = 0;
  charPos = INVALID_INDEX;
  dfaState = nullptr; // Don't delete. It's just a reference.
}

void LexerATNSimulator::SimState::InitializeInstanceFields() {
  index = INVALID_INDEX;
  line = 0;
  charPos = INVALID_INDEX;
}

std::atomic<int> LexerATNSimulator::match_calls(0);


LexerATNSimulator::LexerATNSimulator(const ATN &atn, std::vector<dfa::DFA> &decisionToDFA)
  : LexerATNSimulator(nullptr, atn, decisionToDFA) {
}

LexerATNSimulator::LexerATNSimulator(Lexer *recog, const ATN &atn, std::vector<dfa::DFA> &decisionToDFA)
  : ATNSimulator(atn), _recog(recog), _decisionToDFA(decisionToDFA) {
  InitializeInstanceFields();
}

void LexerATNSimulator::copyState(LexerATNSimulator *simulator) {
  _charPositionInLine = simulator->_charPositionInLine;
  _line = simulator->_line;
  _mode = simulator->_mode;
  _startIndex = simulator->_startIndex;
}

size_t LexerATNSimulator::match(CharStream *input, size_t mode) {
  match_calls.fetch_add(1, std::memory_order_relaxed);
  _mode = mode;
  ssize_t mark = input->mark();

  auto onExit = finally([input, mark] {
    input->release(mark);
  });

  _startIndex = input->index();
  _prevAccept.reset();
  const dfa::DFA &dfa = _decisionToDFA[mode];
  dfa::DFAState* s0 = atn.getLexerStartState(dfa);
  if (s0 == nullptr) {
    return matchATN(input);
  } else {
    return execATN(input, s0);
  }
}

void LexerATNSimulator::reset() {
  _prevAccept.reset();
  _startIndex = 0;
  _line = 1;
  _charPositionInLine = 0;
  _mode = Lexer::DEFAULT_MODE;
}

void LexerATNSimulator::clearDFA() {
  size_t size = _decisionToDFA.size();
  _decisionToDFA.clear();
  for (size_t d = 0; d < size; ++d) {
    _decisionToDFA.emplace_back(atn.getDecisionState(d), d);
  }
}

size_t LexerATNSimulator::matchATN(CharStream *input) {
  ATNState *startState = atn.modeToStartState[_mode];

  ATNConfigSet s0_closure = computeStartState(input, startState);

  bool suppressEdge = s0_closure.hasSemanticContext;
  s0_closure.hasSemanticContext = false;

  dfa::DFAState *next = addDFAState(std::move(s0_closure), suppressEdge);

  size_t predict = execATN(input, next);

  return predict;
}

size_t LexerATNSimulator::execATN(CharStream *input, dfa::DFAState *ds0) {
  if (ds0->isAcceptState) {
    // allow zero-length tokens
    // ml: in Java code this method uses 3 params. The first is a member var of the class anyway (_prevAccept), so why pass it here?
    captureSimState(input, ds0);
  }

  size_t t = input->LA(1);
  dfa::DFAState *s = ds0; // s is current/from DFA state

  while (true) { // while more work
    // As we move src->trg, src->trg, we keep track of the previous trg to
    // avoid looking up the DFA state again, which is expensive.
    // If the previous target was already part of the DFA, we might
    // be able to avoid doing a reach operation upon t. If s!=null,
    // it means that semantic predicates didn't prevent us from
    // creating a DFA state. Once we know s!=null, we check to see if
    // the DFA state has an edge already for t. If so, we can just reuse
    // it's configuration set; there's no point in re-computing it.
    // This is kind of like doing DFA simulation within the ATN
    // simulation because DFA simulation is really just a way to avoid
    // computing reach/closure sets. Technically, once we know that
    // we have a previously added DFA state, we could jump over to
    // the DFA simulator. But, that would mean popping back and forth
    // a lot and making things more complicated algorithmically.
    // This optimization makes a lot of sense for loops within DFA.
    // A character will take us back to an existing DFA state
    // that already has lots of edges out of it. e.g., .* in comments.
    dfa::DFAState *target = getExistingTargetState(s, t);
    if (target == nullptr) {
      target = computeTargetState(input, s, t);
    }

    if (target == ERROR.get()) {
      break;
    }

    // If this is a consumable input element, make sure to consume before
    // capturing the accept state so the input index, line, and char
    // position accurately reflect the state of the interpreter at the
    // end of the token.
    if (t != Token::EOF) {
      consume(input);
    }

    if (target->isAcceptState) {
      captureSimState(input, target);
      if (t == Token::EOF) {
        break;
      }
    }

    t = input->LA(1);
    s = target; // flip; current DFA target becomes new src/from state
  }

  return failOrAccept(input, &s->configs, t);
}

dfa::DFAState *LexerATNSimulator::getExistingTargetState(dfa::DFAState *s, size_t t) {
  return atn.getLexerExistingTargetState(*s, t);
}

dfa::DFAState *LexerATNSimulator::computeTargetState(CharStream *input, dfa::DFAState *s, size_t t) {
  OrderedATNConfigSet reach;

  // if we don't find an existing DFA state
  // Fill reach starting from closure, following t transitions
  getReachableConfigSet(input, &s->configs, &reach, t);

  if (reach.isEmpty()) { // we got nowhere on t from s
    if (!reach.hasSemanticContext) {
      // we got nowhere on t, don't throw out this knowledge; it'd
      // cause a failover from DFA later.
      addDFAEdge(s, t, ERROR.get());
    }

    // stop when we can't match any more char
    return ERROR.get();
  }

  // Add an edge from s to target DFA found/created for reach
  return addDFAEdge(s, t, std::move(reach));
}

size_t LexerATNSimulator::failOrAccept(CharStream *input, ATNConfigSet *reach, size_t t) {
  if (_prevAccept.dfaState != nullptr) {
    accept(input, _prevAccept.dfaState->lexerActionExecutor, _startIndex, _prevAccept.index, _prevAccept.line, _prevAccept.charPos);
    return _prevAccept.dfaState->prediction;
  } else {
    // if no accept and EOF is first char, return EOF
    if (t == Token::EOF && input->index() == _startIndex) {
      return Token::EOF;
    }

    throw LexerNoViableAltException(_recog, input, _startIndex, *reach);
  }
}

void LexerATNSimulator::getReachableConfigSet(CharStream *input, ATNConfigSet *closure_, ATNConfigSet *reach, size_t t) {
  // this is used to skip processing for configs which have a lower priority
  // than a config that already reached an accept state for the same rule
  size_t skipAlt = ATN::INVALID_ALT_NUMBER;

  for (const auto &c : *closure_) {
    bool currentAltReachedAcceptState = c.alt == skipAlt;
    if (currentAltReachedAcceptState && c.hasPassedThroughNonGreedyDecision()) {
      continue;
    }

#if DEBUG_ATN == 1
      std::cout << "testing " << getTokenName((int)t) << " at " << c->toString(true) << std::endl;
#endif

    size_t n = c.state->transitions.size();
    for (size_t ti = 0; ti < n; ti++) { // for each transition
      const AnyTransition &trans = c.state->transitions[ti];
      ATNState *target = getReachableTarget(trans, (int)t);
      if (target != nullptr) {
        const auto &lexerActionExecutor = c.getLexerActionExecutor();
        auto fixedOffsetLexerActionExecutor = lexerActionExecutor.fixOffsetBeforeMatch(static_cast<int>(input->index() - _startIndex));

        bool treatEofAsEpsilon = t == Token::EOF;
        ATNConfig config;
        if (fixedOffsetLexerActionExecutor.has_value()) {
          config = ATNConfig(c, target, std::move(fixedOffsetLexerActionExecutor).value());
        } else {
          config = ATNConfig(c, target, lexerActionExecutor);
        }

        if (closure(input, config, reach, currentAltReachedAcceptState, true, treatEofAsEpsilon)) {
          // any remaining configs for this alt have a lower priority than
          // the one that just reached an accept state.
          skipAlt = c.alt;
          break;
        }
      }
    }
  }
}

void LexerATNSimulator::accept(CharStream *input, const LexerActionExecutor &lexerActionExecutor, size_t /*startIndex*/,
                               size_t index, size_t line, size_t charPos) {
#if DEBUG_ATN == 1
    std::cout << "ACTION ";
    std::cout << toString(lexerActionExecutor) << std::endl;
#endif

  // seek to after last char in token
  input->seek(index);
  _line = line;
  _charPositionInLine = (int)charPos;

  if (_recog != nullptr) {
    lexerActionExecutor.execute(_recog, input, _startIndex);
  }
}

atn::ATNState *LexerATNSimulator::getReachableTarget(const Transition &trans, size_t t) {
  if (trans.matches(t, Lexer::MIN_CHAR_VALUE, Lexer::MAX_CHAR_VALUE)) {
    return trans.getTarget();
  }

  return nullptr;
}

ATNConfigSet LexerATNSimulator::computeStartState(CharStream *input, ATNState *p) {
  AnyPredictionContext initialContext = PredictionContext::empty(); // ml: the purpose of this assignment is unclear
  OrderedATNConfigSet configs;
  for (size_t i = 0; i < p->transitions.size(); i++) {
    ATNState *target = p->transitions[i].getTarget();
    ATNConfig c = ATNConfig(target, (int)(i + 1), initialContext);
    closure(input, c, &configs, false, false, false);
  }

  return configs;
}

bool LexerATNSimulator::closure(CharStream *input, const ATNConfig &config, ATNConfigSet *configs,
                                bool currentAltReachedAcceptState, bool speculative, bool treatEofAsEpsilon) {
#if DEBUG_ATN == 1
    std::cout << "closure(" << config.toString(true) << ")" << std::endl;
#endif

  if (config.state->getStateType() == ATNState::RULE_STOP) {
#if DEBUG_ATN == 1
      if (_recog != nullptr) {
        std::cout << "closure at " << _recog->getRuleNames()[config.state->ruleIndex] << " rule stop " << config << std::endl;
      } else {
        std::cout << "closure at rule stop " << config << std::endl;
      }
#endif

    if (!config.context.valid() || config.context.hasEmptyPath()) {
      if (!config.context.valid() || config.context.isEmpty()) {
        configs->add(config);
        return true;
      } else {
        configs->add(ATNConfig(config, config.state, PredictionContext::empty()));
        currentAltReachedAcceptState = true;
      }
    }

    if (config.context.valid() && !config.context.isEmpty()) {
      for (size_t i = 0; i < config.context.size(); i++) {
        if (config.context.getReturnState(i) != PredictionContext::EMPTY_RETURN_STATE) {
          AnyPredictionContext newContext = config.context.getParent(i); // "pop" return state
          ATNState *returnState = atn.states[config.context.getReturnState(i)].get();
          ATNConfig c = ATNConfig(config, returnState, std::move(newContext));
          currentAltReachedAcceptState = closure(input, c, configs, currentAltReachedAcceptState, speculative, treatEofAsEpsilon);
        }
      }
    }

    return currentAltReachedAcceptState;
  }

  // optimization
  if (!config.state->epsilonOnlyTransitions) {
    if (!currentAltReachedAcceptState || !config.hasPassedThroughNonGreedyDecision()) {
      configs->add(config);
    }
  }

  ATNState *p = config.state;
  for (size_t i = 0; i < p->transitions.size(); i++) {
    const AnyTransition &t = p->transitions[i];
    std::optional<ATNConfig> c = getEpsilonTarget(input, config, t, configs, speculative, treatEofAsEpsilon);
    if (c.has_value()) {
      currentAltReachedAcceptState = closure(input, c.value(), configs, currentAltReachedAcceptState, speculative, treatEofAsEpsilon);
    }
  }

  return currentAltReachedAcceptState;
}

std::optional<ATNConfig> LexerATNSimulator::getEpsilonTarget(CharStream *input, const ATNConfig &config, const Transition &t,
  ATNConfigSet *configs, bool speculative, bool treatEofAsEpsilon) {

  std::optional<ATNConfig> c;
  switch (t.getType()) {
    case TransitionType::RULE: {
      AnyPredictionContext newContext = SingletonPredictionContext::create(config.context, downCast<const RuleTransition&>(t).getFollowState()->stateNumber);
      c = ATNConfig(config, t.getTarget(), std::move(newContext));
      break;
    }

    case TransitionType::PRECEDENCE:
      throw UnsupportedOperationException("Precedence predicates are not supported in lexers.");

    case TransitionType::PREDICATE: {
      /*  Track traversing semantic predicates. If we traverse,
       we cannot add a DFA state for this "reach" computation
       because the DFA would not test the predicate again in the
       future. Rather than creating collections of semantic predicates
       like v3 and testing them on prediction, v4 will test them on the
       fly all the time using the ATN not the DFA. This is slower but
       semantically it's not used that often. One of the key elements to
       this predicate mechanism is not adding DFA states that see
       predicates immediately afterwards in the ATN. For example,

       a : ID {p1}? | ID {p2}? ;

       should create the start state for rule 'a' (to save start state
       competition), but should not create target of ID state. The
       collection of ATN states the following ID references includes
       states reached by traversing predicates. Since this is when we
       test them, we cannot cash the DFA state target of ID.
       */

#if DEBUG_ATN == 1
        std::cout << "EVAL rule " << downCast<const PredicateTransition&>(t).getRuleIndex() << ":" << downCast<const PredicateTransition&>(t).getPredIndex() << std::endl;
#endif

      configs->hasSemanticContext = true;
      if (evaluatePredicate(input, downCast<const PredicateTransition&>(t).getRuleIndex(), downCast<const PredicateTransition&>(t).getPredIndex(), speculative)) {
        c = ATNConfig(config, t.getTarget());
      }
      break;
    }

    case TransitionType::ACTION:
      if (!config.context.valid() || config.context.hasEmptyPath()) {
        // execute actions anywhere in the start rule for a token.
        //
        // TODO: if the entry rule is invoked recursively, some
        // actions may be executed during the recursive call. The
        // problem can appear when hasEmptyPath() is true but
        // isEmpty() is false. In this case, the config needs to be
        // split into two contexts - one with just the empty path
        // and another with everything but the empty path.
        // Unfortunately, the current algorithm does not allow
        // getEpsilonTarget to return two configurations, so
        // additional modifications are needed before we can support
        // the split operation.
        c = ATNConfig(config, t.getTarget(), LexerActionExecutor::append(config.getLexerActionExecutor(),
          atn.lexerActions[downCast<const ActionTransition&>(t).getActionIndex()]));
        break;
      }
      else {
        // ignore actions in referenced rules
        c = ATNConfig(config, t.getTarget());
        break;
      }

    case TransitionType::EPSILON:
      c = ATNConfig(config, t.getTarget());
      break;

    case TransitionType::ATOM:
    case TransitionType::RANGE:
    case TransitionType::SET:
      if (treatEofAsEpsilon) {
        if (t.matches(Token::EOF, Lexer::MIN_CHAR_VALUE, Lexer::MAX_CHAR_VALUE)) {
          c = ATNConfig(config, t.getTarget());
          break;
        }
      }

      break;

    default: // To silence the compiler. Other transition types are not used here.
      break;
  }

  return c;
}

bool LexerATNSimulator::evaluatePredicate(CharStream *input, size_t ruleIndex, size_t predIndex, bool speculative) {
  // assume true if no recognizer was provided
  if (_recog == nullptr) {
    return true;
  }

  if (!speculative) {
    return _recog->sempred(nullptr, ruleIndex, predIndex);
  }

  size_t savedCharPositionInLine = _charPositionInLine;
  size_t savedLine = _line;
  size_t index = input->index();
  ssize_t marker = input->mark();

  auto onExit = finally([this, input, savedCharPositionInLine, savedLine, index, marker] {
    _charPositionInLine = savedCharPositionInLine;
    _line = savedLine;
    input->seek(index);
    input->release(marker);
  });

  consume(input);
  return _recog->sempred(nullptr, ruleIndex, predIndex);
}

void LexerATNSimulator::captureSimState(CharStream *input, dfa::DFAState *dfaState) {
  _prevAccept.index = input->index();
  _prevAccept.line = _line;
  _prevAccept.charPos = _charPositionInLine;
  _prevAccept.dfaState = dfaState;
}

dfa::DFAState *LexerATNSimulator::addDFAEdge(dfa::DFAState *from, size_t t, ATNConfigSet q) {
  /* leading to this call, ATNConfigSet.hasSemanticContext is used as a
   * marker indicating dynamic predicate evaluation makes this edge
   * dependent on the specific input sequence, so the static edge in the
   * DFA should be omitted. The target DFAState is still created since
   * execATN has the ability to resynchronize with the DFA state cache
   * following the predicate evaluation step.
   *
   * TJP notes: next time through the DFA, we see a pred again and eval.
   * If that gets us to a previously created (but dangling) DFA
   * state, we can continue in pure DFA mode from there.
   */
  bool suppressEdge = q.hasSemanticContext;
  q.hasSemanticContext = false;

  dfa::DFAState *to = addDFAState(std::move(q));

  if (suppressEdge) {
    return to;
  }

  addDFAEdge(from, t, to);
  return to;
}

void LexerATNSimulator::addDFAEdge(dfa::DFAState *p, size_t t, dfa::DFAState *q) {
  atn.addLexerDFAEdge(p, t, q);
}

dfa::DFAState *LexerATNSimulator::addDFAState(ATNConfigSet configs) {
  return addDFAState(std::move(configs), true);
}

dfa::DFAState *LexerATNSimulator::addDFAState(ATNConfigSet configs, bool suppressEdge) {
  /* the lexer evaluates predicates on-the-fly; by this point configs
   * should not contain any configurations with unevaluated predicates.
   */
  assert(!configs.hasSemanticContext);

  std::unique_ptr<dfa::DFAState> proposed = std::make_unique<dfa::DFAState>(std::move(configs));
  const ATNConfig *firstConfigWithRuleStopState = nullptr;
  for (const auto &c : proposed->configs) {
    if (c.state->getStateType() == ATNState::RULE_STOP) {
      firstConfigWithRuleStopState = &c;
      break;
    }
  }

  if (firstConfigWithRuleStopState != nullptr) {
    proposed->isAcceptState = true;
    proposed->lexerActionExecutor = firstConfigWithRuleStopState->getLexerActionExecutor();
    proposed->prediction = atn.ruleToTokenType[firstConfigWithRuleStopState->state->ruleIndex];
  }

  return atn.addLexerDFAState(_decisionToDFA[_mode], std::move(proposed), suppressEdge);
}

dfa::DFA& LexerATNSimulator::getDFA(size_t mode) {
  return _decisionToDFA[mode];
}

std::string LexerATNSimulator::getText(CharStream *input) {
  // index is first lookahead char, don't include.
  return input->getText(misc::Interval(_startIndex, input->index() - 1));
}

size_t LexerATNSimulator::getLine() const {
  return _line;
}

void LexerATNSimulator::setLine(size_t line) {
  _line = line;
}

size_t LexerATNSimulator::getCharPositionInLine() {
  return _charPositionInLine;
}

void LexerATNSimulator::setCharPositionInLine(size_t charPositionInLine) {
  _charPositionInLine = charPositionInLine;
}

void LexerATNSimulator::consume(CharStream *input) {
  size_t curChar = input->LA(1);
  if (curChar == '\n') {
    _line++;
    _charPositionInLine = 0;
  } else {
    _charPositionInLine++;
  }
  input->consume();
}

std::string LexerATNSimulator::getTokenName(size_t t) {
  if (t == Token::EOF) {
    return "EOF";
  }
  return std::string("'") + static_cast<char>(t) + std::string("'");
}

void LexerATNSimulator::InitializeInstanceFields() {
  _startIndex = 0;
  _line = 1;
  _charPositionInLine = 0;
  _mode = antlr4::Lexer::DEFAULT_MODE;
}
