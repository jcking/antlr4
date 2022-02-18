/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <type_traits>
#include <utility>
#include <variant>

#include "atn/AbstractPredicateTransition.h"
#include "atn/ActionTransition.h"
#include "atn/AtomTransition.h"
#include "atn/EpsilonTransition.h"
#include "atn/NotSetTransition.h"
#include "atn/PrecedencePredicateTransition.h"
#include "atn/PredicateTransition.h"
#include "atn/RangeTransition.h"
#include "atn/RuleTransition.h"
#include "atn/SetTransition.h"
#include "atn/Transition.h"
#include "atn/WildcardTransition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC AnyTransition final {
  private:
    using Variant = std::variant<ActionTransition, AtomTransition, EpsilonTransition,
                                 NotSetTransition, PrecedencePredicateTransition, PredicateTransition,
                                 RangeTransition, RuleTransition, SetTransition, WildcardTransition>;

    template <typename T>
    static constexpr bool IsVariantAlternative =
        std::conjunction_v<std::is_base_of<Transition, T>, std::negation<std::is_abstract<T>>>;

  public:
    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyTransition(const T &transition)
        : _variant(std::in_place_type<std::decay_t<T>>, transition) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyTransition(T &&transition)
        : _variant(std::in_place_type<std::decay_t<T>>, std::forward<T>(transition)) {}

    AnyTransition(const AnyTransition&) = default;

    AnyTransition(AnyTransition&&) = default;

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyTransition&>
    operator=(const T &transition) {
      _variant.emplace<std::decay_t<T>>(transition);
      return *this;
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyTransition&>
    operator=(T &&transition) {
      _variant.emplace<std::decay_t<T>>(std::forward<T>(transition));
      return *this;
    }

    AnyTransition& operator=(const AnyTransition&) = default;

    AnyTransition& operator=(AnyTransition&&) = default;

    TransitionType getType() const { return get().getType(); }

    ATNState* getTarget() const { return get().getTarget(); }

    void setTarget(ATNState *target) { get().setTarget(target); }

    bool isEpsilon() const { return get().isEpsilon(); }

    const misc::IntervalSet& label() const { return get().label(); }

    bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const { return get().matches(symbol, minVocabSymbol, maxVocabSymbol); }

    bool equals(const Transition &other) const { return get().equals(other); }

    bool equals(const AnyTransition &other) const { return equals(other.get()); }

    std::string toString() const { return get().toString(); }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, bool> is() const {
      return std::holds_alternative<T>(_variant);
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, const T&> as() const {
      return std::get<T>(_variant);
    }

    operator Transition&() { return get(); }

    operator const Transition&() const { return get(); }

  private:
    Transition& get();

    const Transition& get() const;

    Variant _variant;
  };

  inline bool operator==(const AnyTransition &lhs, const AnyTransition &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const AnyTransition &lhs, const Transition &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const Transition &lhs, const AnyTransition &rhs) {
    return rhs.equals(lhs);
  }

  inline bool operator!=(const AnyTransition &lhs, const AnyTransition &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const AnyTransition &lhs, const Transition &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const Transition &lhs, const AnyTransition &rhs) {
    return !operator==(lhs, rhs);
  }

} // namespace atn
} // namespace antlr4
