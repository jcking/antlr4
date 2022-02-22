/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#include "Parser.h"

#include "NoViableAltException.h"

using namespace antlr4;

NoViableAltException::NoViableAltException(Parser *recognizer)
  : NoViableAltException(recognizer, recognizer->getTokenStream(), recognizer->getCurrentToken(),
                         recognizer->getCurrentToken(), atn::ATNConfigSet(), recognizer->getContext()) {
}

NoViableAltException::NoViableAltException(Parser *recognizer, TokenStream *input,Token *startToken,
  Token *offendingToken, atn::ATNConfigSet deadEndConfigs, ParserRuleContext *ctx)
  : RecognitionException("No viable alternative", recognizer, input, ctx, offendingToken),
    _deadEndConfigs(std::move(deadEndConfigs)), _startToken(startToken) {
}

NoViableAltException::~NoViableAltException() {
}

Token* NoViableAltException::getStartToken() const {
  return _startToken;
}

const atn::ATNConfigSet* NoViableAltException::getDeadEndConfigs() const {
  return &_deadEndConfigs;
}
