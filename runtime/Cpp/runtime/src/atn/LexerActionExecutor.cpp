/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "misc/MurmurHash.h"
#include "atn/LexerIndexedCustomAction.h"
#include "support/CPPUtils.h"
#include "support/Arrays.h"

#include "atn/LexerActionExecutor.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::misc;
using namespace antlrcpp;

LexerActionExecutor::LexerActionExecutor(std::vector<AnyLexerAction> lexerActions)
  : _lexerActions(std::move(lexerActions)), _hashCode(generateHashCode()) {
}

LexerActionExecutor LexerActionExecutor::append(const LexerActionExecutor &lexerActionExecutor, AnyLexerAction anyLexerAction) {
  std::vector<AnyLexerAction> lexerActions;
  lexerActions.reserve(lexerActionExecutor._lexerActions.size() + 1);
  lexerActions.insert(lexerActions.end(), lexerActionExecutor._lexerActions.begin(), lexerActionExecutor._lexerActions.end());
  lexerActions.push_back(std::move(anyLexerAction));
  lexerActions.shrink_to_fit();
  return LexerActionExecutor(std::move(lexerActions));
}

std::optional<LexerActionExecutor> LexerActionExecutor::fixOffsetBeforeMatch(int offset) const {
  std::vector<AnyLexerAction> updatedLexerActions;
  for (size_t i = 0; i < _lexerActions.size(); i++) {
    if (_lexerActions[i].isPositionDependent() && !_lexerActions[i].is<LexerIndexedCustomAction>()) {
      if (updatedLexerActions.empty()) {
        updatedLexerActions = _lexerActions; // Make a copy.
      }

      updatedLexerActions[i] = AnyLexerAction(LexerIndexedCustomAction(offset, _lexerActions[i].getShared()));
    }
  }

  if (updatedLexerActions.empty()) {
    return std::nullopt;
  }

  return LexerActionExecutor(std::move(updatedLexerActions));
}

const std::vector<AnyLexerAction>& LexerActionExecutor::getLexerActions() const {
  return _lexerActions;
}

void LexerActionExecutor::execute(Lexer *lexer, CharStream *input, size_t startIndex) const {
  bool requiresSeek = false;
  size_t stopIndex = input->index();

  auto onExit = finally([requiresSeek, input, stopIndex]() {
    if (requiresSeek) {
      input->seek(stopIndex);
    }
  });
  for (const auto &lexerAction : _lexerActions) {
    if (lexerAction.is<LexerIndexedCustomAction>()) {
      int offset = lexerAction.as<LexerIndexedCustomAction>().getOffset();
      input->seek(startIndex + offset);
      requiresSeek = (startIndex + offset) != stopIndex;
    } else if (lexerAction.isPositionDependent()) {
      input->seek(stopIndex);
      requiresSeek = false;
    }
    lexerAction.execute(lexer);
  }
}

size_t LexerActionExecutor::hashCode() const {
  return _hashCode;
}

bool LexerActionExecutor::equals(const LexerActionExecutor &other) const {
  if (&other == this) {
    return true;
  }

  return _hashCode == other._hashCode && Arrays::equals(_lexerActions, other._lexerActions);
}

size_t LexerActionExecutor::generateHashCode() const {
  size_t hash = MurmurHash::initialize();
  for (const auto &lexerAction : _lexerActions) {
    hash = MurmurHash::update(hash, lexerAction);
  }
  hash = MurmurHash::finish(hash, _lexerActions.size());

  return hash;
}
