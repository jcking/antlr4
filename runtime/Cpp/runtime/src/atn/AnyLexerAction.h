/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "atn/LexerAction.h"
#include "atn/LexerActionType.h"
#include "atn/LexerChannelAction.h"
#include "atn/LexerCustomAction.h"
#include "atn/LexerIndexedCustomAction.h"
#include "atn/LexerModeAction.h"
#include "atn/LexerMoreAction.h"
#include "atn/LexerPopModeAction.h"
#include "atn/LexerPushModeAction.h"
#include "atn/LexerSkipAction.h"
#include "atn/LexerTypeAction.h"

namespace antlr4 {
namespace atn {

  class LexerActionExecutor;

  class ANTLR4CPP_PUBLIC AnyLexerAction final {
  private:
    using Variant = std::variant<LexerChannelAction, LexerCustomAction, LexerIndexedCustomAction,
                                 LexerModeAction, LexerMoreAction, LexerPopModeAction,
                                 LexerPushModeAction, LexerSkipAction, LexerTypeAction>;

    template <typename T>
    static constexpr bool IsVariantAlternative =
        std::conjunction_v<std::is_base_of<LexerAction, T>, std::is_final<T>>;

  public:
    AnyLexerAction() = delete;

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyLexerAction(const T &lexerAction)
        : _variant(std::in_place_type<std::decay_t<T>>, lexerAction) {}

    template <typename T, typename = std::enable_if_t<IsVariantAlternative<std::decay_t<T>>>>
    AnyLexerAction(T &&lexerAction)
        : _variant(std::in_place_type<std::decay_t<T>>, std::forward<T>(lexerAction)) {}

    AnyLexerAction(const AnyLexerAction&) = default;

    AnyLexerAction(AnyLexerAction&&) = default;

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyLexerAction&>
    operator=(const T &lexerAction) {
      _variant.emplace<std::decay_t<T>>(lexerAction);
      return *this;
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<std::decay_t<T>>, AnyLexerAction&>
    operator=(T &&lexerAction) {
      _variant.emplace<std::decay_t<T>>(std::forward<T>(lexerAction));
      return *this;
    }

    AnyLexerAction& operator=(const AnyLexerAction&) = default;

    AnyLexerAction& operator=(AnyLexerAction&&) = default;

    LexerActionType getActionType() const { return get().getActionType(); }

    bool isPositionDependent() const { return get().isPositionDependent(); }

    void execute(Lexer *lexer) const { get().execute(lexer); }

    size_t hashCode() const { return get().hashCode(); }

    bool equals(const LexerAction &other) const { return get().equals(other); }

    bool equals(const AnyLexerAction &other) const { return equals(other.get()); }

    std::string toString() const { return get().toString(); }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, bool> is() const {
      return std::holds_alternative<T>(_variant);
    }

    template <typename T>
    std::enable_if_t<IsVariantAlternative<T>, const T&> as() const {
      return std::get<T>(_variant);
    }

  private:
    friend class LexerActionExecutor;

    const LexerAction& get() const;

    std::shared_ptr<const LexerAction> getShared() const;

    Variant _variant;
  };

  inline bool operator==(const AnyLexerAction &lhs, const AnyLexerAction &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const AnyLexerAction &lhs, const LexerAction &rhs) {
    return lhs.equals(rhs);
  }

  inline bool operator==(const LexerAction &lhs, const AnyLexerAction &rhs) {
    return rhs.equals(lhs);
  }

  inline bool operator!=(const AnyLexerAction &lhs, const AnyLexerAction &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const AnyLexerAction &lhs, const LexerAction &rhs) {
    return !operator==(lhs, rhs);
  }

  inline bool operator!=(const LexerAction &lhs, const AnyLexerAction &rhs) {
    return !operator==(lhs, rhs);
  }

} // namespace atn
} // namespace antlr4

namespace std {

  template <> struct
  hash<::antlr4::atn::AnyLexerAction> {
    size_t operator()(const ::antlr4::atn::AnyLexerAction &anyLexerAction) const {
      return anyLexerAction.hashCode();
    }
  };

} // namespace std
