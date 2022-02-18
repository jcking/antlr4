/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#include "atn/SemanticContext.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC AnySemanticContext final {
  private:
    using Variant = std::variant<std::monostate, SemanticContext::Predicate, SemanticContext::PrecedencePredicate,
                                 SemanticContext::AND, SemanticContext::OR>;

    template <typename T>
    static constexpr bool IsVariantAlternative =
        std::conjunction_v<std::is_base_of<SemanticContext, T>, std::is_final<T>>;

  public:
    AnySemanticContext() : _variant(std::in_place_index<0>) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnySemanticContext(const T &semanticContext)
        : _variant(std::in_place_type<std::decay_t<T>>, semanticContext) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnySemanticContext(T &&semanticContext)
        : _variant(std::in_place_type<std::decay_t<T>>, std::forward<T>(semanticContext)) {}

    AnySemanticContext(const AnySemanticContext&) = default;

    AnySemanticContext(AnySemanticContext&&) = default;

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnySemanticContext&>
    operator=(const T &semanticContext) {
      _variant.emplace<std::decay_t<T>>(semanticContext);
      return *this;
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnySemanticContext&>
    operator=(T &&semanticContext) {
      _variant.emplace<std::decay_t<T>>(std::forward<T>(semanticContext));
      return *this;
    }

    AnySemanticContext& operator=(const AnySemanticContext&) = default;

    AnySemanticContext& operator=(AnySemanticContext&&) = default;

    bool valid() const { return _variant.index() != 0; }

    SemanticContextType getType() const { return get().getType(); }

    bool eval(Recognizer *parser, RuleContext *parserCallStack) const { return get().eval(parser, parserCallStack); }

    AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const { return get().evalPrecedence(parser, parserCallStack); }

    size_t hashCode() const { return get().hashCode(); }

    bool equals(const SemanticContext &other) const { return get().equals(other); }

    bool equals(const AnySemanticContext &other) const { return equals(other.get()); }

    std::string toString() const { return get().toString(); }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, bool> is() const {
      return std::holds_alternative<T>(_variant);
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, const T&> as() const {
      return std::get<T>(_variant);
    }

    explicit operator bool() const { return valid(); }

    bool operator!() const { return !valid(); }

  private:
    const SemanticContext& get() const;

    Variant _variant;
  };

  inline bool operator==(const AnySemanticContext &lhs, const AnySemanticContext &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const AnySemanticContext &lhs, const SemanticContext &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const SemanticContext &lhs, const AnySemanticContext &rhs) {
    return rhs.equals(lhs);
  }

  inline bool operator!=(const AnySemanticContext &lhs, const AnySemanticContext &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const AnySemanticContext &lhs, const SemanticContext &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const SemanticContext &lhs, const AnySemanticContext &rhs) {
    return !operator==(lhs, rhs);
  }

} // namespace atn
} // namespace antlr4

namespace std {

template <>
struct hash<::antlr4::atn::AnySemanticContext> {
  size_t operator()(const ::antlr4::atn::AnySemanticContext &anySemanticContext) const {
    return anySemanticContext.hashCode();
  }
};

} // namespace std
