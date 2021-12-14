#include "XPathLexer.h"

using namespace antlr4::tree::xpath;

XPathLexer::XPathLexer(CharStream *input) : Lexer(input) {
  _interpreter = new atn::LexerATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

XPathLexer::~XPathLexer() { delete _interpreter; }

std::string XPathLexer::getGrammarFileName() const { return "XPathLexer.g4"; }

const std::vector<std::string> &XPathLexer::getRuleNames() const { return _ruleNames; }

const std::vector<std::string> &XPathLexer::getChannelNames() const { return _channelNames; }

const std::vector<std::string> &XPathLexer::getModeNames() const { return _modeNames; }

antlr4::dfa::Vocabulary &XPathLexer::getVocabulary() const { return _vocabulary; }

const std::vector<uint16_t> XPathLexer::getSerializedATN() const { return _serializedATN; }

const antlr4::atn::ATN &XPathLexer::getATN() const { return _atn; }

void XPathLexer::action(RuleContext *context, size_t ruleIndex, size_t actionIndex) {
  switch (ruleIndex) {
  case 4:
    IDAction(dynamic_cast<antlr4::RuleContext *>(context), actionIndex);
    break;

  default:
    break;
  }
}

void XPathLexer::IDAction(antlr4::RuleContext * /*context*/, size_t actionIndex) {
  switch (actionIndex) {
  case 0:
    if (isupper(getText()[0]))
      setType(TOKEN_REF);
    else
      setType(RULE_REF);
    break;

  default:
    break;
  }
}

// Static vars and initialization.
std::vector<antlr4::dfa::DFA> XPathLexer::_decisionToDFA;
antlr4::atn::PredictionContextCache XPathLexer::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
antlr4::atn::ATN XPathLexer::_atn;
std::vector<uint16_t> XPathLexer::_serializedATN;

std::vector<std::string> XPathLexer::_ruleNames = {"ANYWHERE", "ROOT",     "WILDCARD",      "BANG",
                                                   "ID",       "NameChar", "NameStartChar", "STRING"};

std::vector<std::string> XPathLexer::_channelNames = {"DEFAULT_TOKEN_CHANNEL", "HIDDEN"};

std::vector<std::string> XPathLexer::_modeNames = {"DEFAULT_MODE"};

std::vector<std::string> XPathLexer::_literalNames = {"", "", "", "'//'", "'/'", "'*'", "'!'"};

std::vector<std::string> XPathLexer::_symbolicNames = {"",         "TOKEN_REF", "RULE_REF", "ANYWHERE", "ROOT",
                                                       "WILDCARD", "BANG",      "ID",       "STRING"};

antlr4::dfa::Vocabulary XPathLexer::_vocabulary(_literalNames, _symbolicNames);

XPathLexer::Initializer::Initializer() {
  _serializedATN = {
      0x3,    0x430,  0xd6d1, 0x8206, 0xad2d, 0x4417, 0xaef1, 0x8d80, 0xaadd, 0x2,    0xa,    0x34,   0x8,   0x1,
      0x4,    0x2,    0x9,    0x2,    0x4,    0x3,    0x9,    0x3,    0x4,    0x4,    0x9,    0x4,    0x4,   0x5,
      0x9,    0x5,    0x4,    0x6,    0x9,    0x6,    0x4,    0x7,    0x9,    0x7,    0x4,    0x8,    0x9,   0x8,
      0x4,    0x9,    0x9,    0x9,    0x3,    0x2,    0x3,    0x2,    0x3,    0x2,    0x3,    0x3,    0x3,   0x3,
      0x3,    0x4,    0x3,    0x4,    0x3,    0x5,    0x3,    0x5,    0x3,    0x6,    0x3,    0x6,    0x7,   0x6,
      0x1f,   0xa,    0x6,    0xc,    0x6,    0xe,    0x6,    0x22,   0xb,    0x6,    0x3,    0x6,    0x3,   0x6,
      0x3,    0x7,    0x3,    0x7,    0x5,    0x7,    0x28,   0xa,    0x7,    0x3,    0x8,    0x3,    0x8,   0x3,
      0x9,    0x3,    0x9,    0x7,    0x9,    0x2e,   0xa,    0x9,    0xc,    0x9,    0xe,    0x9,    0x31,  0xb,
      0x9,    0x3,    0x9,    0x3,    0x9,    0x3,    0x2f,   0x2,    0xa,    0x3,    0x5,    0x5,    0x6,   0x7,
      0x7,    0x9,    0x8,    0xb,    0x9,    0xd,    0x2,    0xf,    0x2,    0x11,   0xa,    0x3,    0x2,   0x4,
      0x7,    0x2,    0x32,   0x3b,   0x61,   0x61,   0xb9,   0xb9,   0x302,  0x371,  0x2041, 0x2042, 0xf,   0x2,
      0x43,   0x5c,   0x63,   0x7c,   0xc2,   0xd8,   0xda,   0xf8,   0xfa,   0x301,  0x372,  0x37f,  0x381, 0x2001,
      0x200e, 0x200f, 0x2072, 0x2191, 0x2c02, 0x2ff1, 0x3003, 0xd801, 0xf902, 0xfdd1, 0xfdf2, 0x1,    0x34,  0x2,
      0x3,    0x3,    0x2,    0x2,    0x2,    0x2,    0x5,    0x3,    0x2,    0x2,    0x2,    0x2,    0x7,   0x3,
      0x2,    0x2,    0x2,    0x2,    0x9,    0x3,    0x2,    0x2,    0x2,    0x2,    0xb,    0x3,    0x2,   0x2,
      0x2,    0x2,    0x11,   0x3,    0x2,    0x2,    0x2,    0x3,    0x13,   0x3,    0x2,    0x2,    0x2,   0x5,
      0x16,   0x3,    0x2,    0x2,    0x2,    0x7,    0x18,   0x3,    0x2,    0x2,    0x2,    0x9,    0x1a,  0x3,
      0x2,    0x2,    0x2,    0xb,    0x1c,   0x3,    0x2,    0x2,    0x2,    0xd,    0x27,   0x3,    0x2,   0x2,
      0x2,    0xf,    0x29,   0x3,    0x2,    0x2,    0x2,    0x11,   0x2b,   0x3,    0x2,    0x2,    0x2,   0x13,
      0x14,   0x7,    0x31,   0x2,    0x2,    0x14,   0x15,   0x7,    0x31,   0x2,    0x2,    0x15,   0x4,   0x3,
      0x2,    0x2,    0x2,    0x16,   0x17,   0x7,    0x31,   0x2,    0x2,    0x17,   0x6,    0x3,    0x2,   0x2,
      0x2,    0x18,   0x19,   0x7,    0x2c,   0x2,    0x2,    0x19,   0x8,    0x3,    0x2,    0x2,    0x2,   0x1a,
      0x1b,   0x7,    0x23,   0x2,    0x2,    0x1b,   0xa,    0x3,    0x2,    0x2,    0x2,    0x1c,   0x20,  0x5,
      0xf,    0x8,    0x2,    0x1d,   0x1f,   0x5,    0xd,    0x7,    0x2,    0x1e,   0x1d,   0x3,    0x2,   0x2,
      0x2,    0x1f,   0x22,   0x3,    0x2,    0x2,    0x2,    0x20,   0x1e,   0x3,    0x2,    0x2,    0x2,   0x20,
      0x21,   0x3,    0x2,    0x2,    0x2,    0x21,   0x23,   0x3,    0x2,    0x2,    0x2,    0x22,   0x20,  0x3,
      0x2,    0x2,    0x2,    0x23,   0x24,   0x8,    0x6,    0x2,    0x2,    0x24,   0xc,    0x3,    0x2,   0x2,
      0x2,    0x25,   0x28,   0x5,    0xf,    0x8,    0x2,    0x26,   0x28,   0x9,    0x2,    0x2,    0x2,   0x27,
      0x25,   0x3,    0x2,    0x2,    0x2,    0x27,   0x26,   0x3,    0x2,    0x2,    0x2,    0x28,   0xe,   0x3,
      0x2,    0x2,    0x2,    0x29,   0x2a,   0x9,    0x3,    0x2,    0x2,    0x2a,   0x10,   0x3,    0x2,   0x2,
      0x2,    0x2b,   0x2f,   0x7,    0x29,   0x2,    0x2,    0x2c,   0x2e,   0xb,    0x2,    0x2,    0x2,   0x2d,
      0x2c,   0x3,    0x2,    0x2,    0x2,    0x2e,   0x31,   0x3,    0x2,    0x2,    0x2,    0x2f,   0x30,  0x3,
      0x2,    0x2,    0x2,    0x2f,   0x2d,   0x3,    0x2,    0x2,    0x2,    0x30,   0x32,   0x3,    0x2,   0x2,
      0x2,    0x31,   0x2f,   0x3,    0x2,    0x2,    0x2,    0x32,   0x33,   0x7,    0x29,   0x2,    0x2,   0x33,
      0x12,   0x3,    0x2,    0x2,    0x2,    0x6,    0x2,    0x20,   0x27,   0x2f,   0x3,    0x3,    0x6,   0x2,
  };

  antlr4::atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) {
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

XPathLexer::Initializer XPathLexer::_init;
