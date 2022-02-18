/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "misc/MurmurHash.h"
#include "support/CPPUtils.h"
#include "support/Arrays.h"
#include "support/Casts.h"
#include "atn/AnySemanticContext.h"

#include "SemanticContext.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlrcpp;

namespace {

std::vector<SemanticContext::PrecedencePredicate> filterPrecedencePredicates(const std::unordered_set<AnySemanticContext> &collection) {
  std::vector<SemanticContext::PrecedencePredicate> precedencePredicates;
  for (const auto &context : collection) {
    if (context.is<SemanticContext::PrecedencePredicate>()) {
      precedencePredicates.push_back(context.as<SemanticContext::PrecedencePredicate>());
    }
  }
  return precedencePredicates;
}

}

//------------------ Predicate -----------------------------------------------------------------------------------------

SemanticContext::Predicate::Predicate(size_t ruleIndex, size_t predIndex, bool isCtxDependent)
    : _ruleIndex(ruleIndex), _predIndex(predIndex), _isCtxDependent(isCtxDependent) {}

SemanticContextType SemanticContext::Predicate::getType() const {
  return SemanticContextType::PREDICATE;
}

bool SemanticContext::Predicate::eval(Recognizer *parser, RuleContext *parserCallStack) const {
  RuleContext *localctx = nullptr;
  if (isCtxDependent()) {
    localctx = parserCallStack;
  }
  return parser->sempred(localctx, getRuleIndex(), getPredIndex());
}

AnySemanticContext SemanticContext::Predicate::evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const {
  static_cast<void>(parser);
  static_cast<void>(parserCallStack);
  return AnySemanticContext(*this);
}

size_t SemanticContext::Predicate::hashCode() const {
  size_t hashCode = misc::MurmurHash::initialize();
  hashCode = misc::MurmurHash::update(hashCode, getType());
  hashCode = misc::MurmurHash::update(hashCode, getRuleIndex());
  hashCode = misc::MurmurHash::update(hashCode, getPredIndex());
  hashCode = misc::MurmurHash::update(hashCode, isCtxDependent() ? 1 : 0);
  hashCode = misc::MurmurHash::finish(hashCode, 4);
  return hashCode;
}

bool SemanticContext::Predicate::equals(const SemanticContext &other) const {
  if (this == &other) {
    return true;
  }
  if (getType() != other.getType()) {
    return false;
  }
  const Predicate &that = downCast<const Predicate&>(other);
  return getRuleIndex() == that.getRuleIndex() && getPredIndex() == that.getPredIndex() && isCtxDependent() == that.isCtxDependent();
}

std::string SemanticContext::Predicate::toString() const {
  return std::string("{") + std::to_string(getRuleIndex()) + std::string(":") + std::to_string(getPredIndex()) + std::string("}?");
}

//------------------ PrecedencePredicate -------------------------------------------------------------------------------

SemanticContext::PrecedencePredicate::PrecedencePredicate(int precedence) : _precedence(precedence) {}

SemanticContextType SemanticContext::PrecedencePredicate::getType() const {
  return SemanticContextType::PRECEDENCE_PREDICATE;
}

bool SemanticContext::PrecedencePredicate::eval(Recognizer *parser, RuleContext *parserCallStack) const {
  return parser->precpred(parserCallStack, getPrecedence());
}

AnySemanticContext SemanticContext::PrecedencePredicate::evalPrecedence(Recognizer *parser,
  RuleContext *parserCallStack) const {
  if (parser->precpred(parserCallStack, getPrecedence())) {
    return SemanticContext::none();
  } else {
    return AnySemanticContext();
  }
}

size_t SemanticContext::PrecedencePredicate::hashCode() const {
  size_t hashCode = misc::MurmurHash::initialize();
  hashCode = misc::MurmurHash::update(hashCode, getType());
  hashCode = misc::MurmurHash::update(hashCode, getPrecedence());
  hashCode = misc::MurmurHash::finish(hashCode, 2);
  return hashCode;
}

bool SemanticContext::PrecedencePredicate::equals(const SemanticContext &other) const {
  if (this == &other) {
    return true;
  }
  if (getType() != other.getType()) {
    return false;
  }
  const PrecedencePredicate &that = downCast<const PrecedencePredicate&>(other);
  return getPrecedence() == that.getPrecedence();
}

std::string SemanticContext::PrecedencePredicate::toString() const {
  return "{" + std::to_string(getPrecedence()) + ">=prec}?";
}

//------------------ AND -----------------------------------------------------------------------------------------------

SemanticContext::AND::AND(AnySemanticContext lhs, AnySemanticContext rhs) : _operands(std::make_shared<std::vector<AnySemanticContext>>()) {
  std::unordered_set<AnySemanticContext> operands;

  if (lhs.is<AND>()) {
    for (const auto &operand : lhs.as<AND>().getOperands()) {
      operands.insert(operand);
    }
  } else {
    operands.insert(std::move(lhs));
  }

  if (rhs.is<AND>()) {
    for (const auto &operand : rhs.as<AND>().getOperands()) {
      operands.insert(operand);
    }
  } else {
    operands.insert(std::move(rhs));
  }

  {
    std::vector<PrecedencePredicate> precedencePredicates = filterPrecedencePredicates(operands);
    if (!precedencePredicates.empty()) {
      auto reduced = std::min_element(precedencePredicates.begin(), precedencePredicates.end());
      operands.insert(*reduced);
    }
  }

  _operands->reserve(operands.size());
  for (auto &operand : operands) {
    _operands->push_back(std::move(operand));
  }
  _operands->shrink_to_fit();
}

const std::vector<AnySemanticContext>& SemanticContext::AND::getOperands() const {
  return *_operands;
}

SemanticContextType SemanticContext::AND::getType() const {
  return SemanticContextType::AND;
}

bool SemanticContext::AND::eval(Recognizer *parser, RuleContext *parserCallStack) const {
  for (const auto &operand : getOperands()) {
    if (!operand.eval(parser, parserCallStack)) {
      return false;
    }
  }
  return true;
}

AnySemanticContext SemanticContext::AND::evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const {
  bool differs = false;
  std::vector<AnySemanticContext> operands;
  for (const auto &context : getOperands()) {
    AnySemanticContext evaluated = context.evalPrecedence(parser, parserCallStack);
    differs |= (evaluated != context);
    if (!evaluated.valid()) {
      // The AND context is false if any element is false.
      return AnySemanticContext();
    } else if (evaluated != none()) {
      if (operands.empty()) {
        operands.reserve(getOperands().size());
      }
      // Reduce the result by skipping true elements.
      operands.push_back(std::move(evaluated));
    }
  }

  if (!differs) {
    return *this;
  }

  if (operands.empty()) {
    // All elements were true, so the AND context is true.
    return none();
  }

  AnySemanticContext result = std::move(operands[0]);
  for (size_t index = 1; index < operands.size(); index++) {
    result = SemanticContext::And(std::move(result), std::move(operands[index]));
  }

  return result;
}

size_t SemanticContext::AND::hashCode() const {
  size_t hashCode = misc::MurmurHash::initialize();
  hashCode = misc::MurmurHash::update(hashCode, getType());
  hashCode = misc::MurmurHash::update(hashCode, getOperands());
  hashCode = misc::MurmurHash::finish(hashCode, 2);
  return hashCode;
}

bool SemanticContext::AND::equals(const SemanticContext &other) const {
  if (this == &other) {
    return true;
  }
  if (getType() != other.getType()) {
    return false;
  }
  const AND &that = downCast<const AND&>(other);
  return Arrays::equals(getOperands(), that.getOperands());
}

std::string SemanticContext::AND::toString() const {
  std::string tmp;
  if (!_operands->empty()) {
    tmp.push_back('(');
    for (size_t index = 0; index < _operands->size(); index++) {
      if (index != 0) {
        tmp.append(" && ");
      }
      tmp.append((*_operands)[index].toString());
    }
    tmp.push_back(')');
  }
  return tmp;
}

//------------------ OR ------------------------------------------------------------------------------------------------

SemanticContext::OR::OR(AnySemanticContext lhs, AnySemanticContext rhs) : _operands(std::make_shared<std::vector<AnySemanticContext>>()) {
  std::unordered_set<AnySemanticContext> operands;

  if (lhs.is<OR>()) {
    for (const auto &operand : lhs.as<OR>().getOperands()) {
      operands.insert(operand);
    }
  } else {
    operands.insert(std::move(lhs));
  }

  if (rhs.is<OR>()) {
    for (const auto &operand : rhs.as<OR>().getOperands()) {
      operands.insert(operand);
    }
  } else {
    operands.insert(std::move(rhs));
  }

  {
    std::vector<PrecedencePredicate> precedencePredicates = filterPrecedencePredicates(operands);
    if (!precedencePredicates.empty()) {
      auto reduced = std::min_element(precedencePredicates.begin(), precedencePredicates.end());
      operands.insert(*reduced);
    }
  }

  _operands->reserve(operands.size());
  for (auto &operand : operands) {
    _operands->push_back(std::move(operand));
  }
  _operands->shrink_to_fit();
}

const std::vector<AnySemanticContext>& SemanticContext::OR::getOperands() const {
  return *_operands;
}

SemanticContextType SemanticContext::OR::getType() const {
  return SemanticContextType::OR;
}

bool SemanticContext::OR::eval(Recognizer *parser, RuleContext *parserCallStack) const {
  for (const auto &operand : getOperands()) {
    if (operand.eval(parser, parserCallStack)) {
      return true;
    }
  }
  return false;
}

AnySemanticContext SemanticContext::OR::evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const {
  bool differs = false;
  std::vector<AnySemanticContext> operands;
  for (const auto &context : getOperands()) {
    AnySemanticContext evaluated = context.evalPrecedence(parser, parserCallStack);
    differs |= (evaluated != context);
    if (evaluated == none()) {
      // The OR context is true if any element is true.
      return none();
    } else if (evaluated.valid()) {
      if (operands.empty()) {
        operands.reserve(getOperands().size());
      }
      // Reduce the result by skipping false elements.
      operands.push_back(std::move(evaluated));
    }
  }

  if (!differs) {
    return *this;
  }

  if (operands.empty()) {
    // All elements were false, so the OR context is false.
    return AnySemanticContext();
  }

  AnySemanticContext result = std::move(operands[0]);
  for (size_t index = 1; index < operands.size(); index++) {
    result = SemanticContext::Or(std::move(result), std::move(operands[index]));
  }

  return result;
}

size_t SemanticContext::OR::hashCode() const {
  size_t hashCode = misc::MurmurHash::initialize();
  hashCode = misc::MurmurHash::update(hashCode, getType());
  hashCode = misc::MurmurHash::update(hashCode, getOperands());
  hashCode = misc::MurmurHash::finish(hashCode, 2);
  return hashCode;
}

bool SemanticContext::OR::equals(const SemanticContext &other) const {
  if (this == &other) {
    return true;
  }
  if (getType() != other.getType()) {
    return false;
  }
  const OR &that = downCast<const OR&>(other);
  return Arrays::equals(getOperands(), that.getOperands());
}

std::string SemanticContext::OR::toString() const {
  std::string tmp;
  if (!_operands->empty()) {
    tmp.push_back('(');
    for (size_t index = 0; index < _operands->size(); index++) {
      if (index != 0) {
        tmp.append(" || ");
      }
      tmp.append((*_operands)[index].toString());
    }
    tmp.push_back(')');
  }
  return tmp;
}

//------------------ SemanticContext -----------------------------------------------------------------------------------

AnySemanticContext SemanticContext::none() {
  return Predicate(INVALID_INDEX, INVALID_INDEX, false);
}

AnySemanticContext SemanticContext::And(AnySemanticContext lhs, AnySemanticContext rhs) {
  if (!lhs.valid() || lhs == none()) {
    return rhs;
  }

  if (!rhs.valid() || rhs == none()) {
    return lhs;
  }

  AND result(std::move(lhs), std::move(rhs));
  if (result.getOperands().size() == 1) {
    return result.getOperands()[0];
  }

  return result;
}

AnySemanticContext SemanticContext::Or(AnySemanticContext lhs, AnySemanticContext rhs) {
  if (!lhs.valid()) {
    return lhs;
  }
  if (!rhs.valid()) {
    return rhs;
  }

  if (lhs == none() || rhs == none()) {
    return none();
  }

  OR result(std::move(lhs), std::move(rhs));
  if (result.getOperands().size() == 1) {
    return result.getOperands()[0];
  }

  return result;
}

