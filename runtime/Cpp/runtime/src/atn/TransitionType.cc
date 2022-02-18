#include "atn/TransitionType.h"

using namespace antlr4::atn;

std::string_view transitionName(TransitionType transitionType) {
  switch (transitionType) {
    case TransitionType::EPSILON:
      return "EPSILON";
    case TransitionType::RANGE:
      return "RANGE";
    case TransitionType::RULE:
      return "RULE";
    case TransitionType::PREDICATE:
      return "PREDICATE";
    case TransitionType::ATOM:
      return "ATOM";
    case TransitionType::ACTION:
      return "ACTION";
    case TransitionType::SET:
      return "SET";
    case TransitionType::NOT_SET:
      return "NOT_SET";
    case TransitionType::WILDCARD:
      return "WILDCARD";
    case TransitionType::PRECEDENCE:
      return "PRECEDENCE";
    default:
      return "INVALID";
  }
}
