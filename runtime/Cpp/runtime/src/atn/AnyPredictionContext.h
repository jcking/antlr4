/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#include "atn/PredictionContext.h"
#include "atn/ArrayPredictionContext.h"
#include "atn/EmptyPredictionContext.h"
#include "atn/SingletonPredictionContext.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC AnyPredictionContext final {
  private:
    using Variant = std::variant<std::monostate, SingletonPredictionContext, ArrayPredictionContext, EmptyPredictionContext>;

    template <typename T>
    static constexpr bool IsVariantAlternative =
        std::conjunction_v<std::is_base_of<PredictionContext, T>, std::negation<std::is_abstract<T>>>;

  public:
    AnyPredictionContext() : _variant(std::in_place_index<0>) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyPredictionContext(const T &predictionContext)
        : _variant(std::in_place_type<std::decay_t<T>>, predictionContext) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyPredictionContext(T &&predictionContext)
        : _variant(std::in_place_type<std::decay_t<T>>, std::forward<T>(predictionContext)) {}

    AnyPredictionContext(const AnyPredictionContext&) = default;

    AnyPredictionContext(AnyPredictionContext&&) = default;

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyPredictionContext&>
    operator=(const T &predictionContext) {
      _variant.emplace<std::decay_t<T>>(predictionContext);
      return *this;
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyPredictionContext&>
    operator=(T &&predictionContext) {
      _variant.emplace<std::decay_t<T>>(std::forward<T>(predictionContext));
      return *this;
    }

    AnyPredictionContext& operator=(const AnyPredictionContext&) = default;

    AnyPredictionContext& operator=(AnyPredictionContext&&) = default;

    bool valid() const { return _variant.index() != 0; }

    PredictionContextType getType() const { return get().getType(); }

    size_t size() const { return get().size(); }

    const AnyPredictionContext& getParent(size_t index) const { return get().getParent(index); }

    size_t getReturnState(size_t index) const { return get().getReturnState(index); }

    bool isEmpty() const { return get().isEmpty(); }

    bool hasEmptyPath() const { return get().hasEmptyPath(); }

    size_t hashCode() const { return valid() ? get().hashCode() : 0; }

    bool equals(const PredictionContext &other) const { return valid() && get().equals(other); }

    bool equals(const AnyPredictionContext &other) const { return valid() == other.valid() && (!valid() || get().equals(other.get())); }

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

  private:
    friend class PredictionContext;

    const PredictionContext& get() const;

    Variant _variant;
  };

  inline bool operator==(const AnyPredictionContext &lhs, const AnyPredictionContext &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const AnyPredictionContext &lhs, const PredictionContext &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const PredictionContext &lhs, const AnyPredictionContext &rhs) {
    return rhs.equals(lhs);
  }

  inline bool operator!=(const AnyPredictionContext &lhs, const AnyPredictionContext &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const AnyPredictionContext &lhs, const PredictionContext &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const PredictionContext &lhs, const AnyPredictionContext &rhs) {
    return !operator==(lhs, rhs);
  }

} // namespace atn
} // namespace antlr4

namespace std {

  template <>
  struct hash<::antlr4::atn::AnyPredictionContext> {
    size_t operator()(const ::antlr4::atn::AnyPredictionContext &anyPredictionContext) const {
      return anyPredictionContext.hashCode();
    }
  };

} // namespace std
