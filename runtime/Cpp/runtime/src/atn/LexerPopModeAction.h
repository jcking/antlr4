/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/LexerAction.h"
#include "atn/LexerActionType.h"

namespace antlr4 {
namespace atn {

  /// <summary>
  /// Implements the {@code popMode} lexer action by calling <seealso cref="Lexer#popMode"/>.
  ///
  /// <para>The {@code popMode} command does not have any parameters, so this action is
  /// implemented as a singleton instance exposed by <seealso cref="#INSTANCE"/>.</para>
  ///
  /// @author Sam Harwell
  /// @since 4.2
  /// </summary>
  class ANTLR4CPP_PUBLIC LexerPopModeAction final : public LexerAction {
  public:
    /// Constructs an instance of the lexer {@code popMode} command.
    LexerPopModeAction() = default;

    LexerPopModeAction(const LexerPopModeAction&) = default;

    LexerPopModeAction(LexerPopModeAction&&) = default;

    LexerPopModeAction& operator=(const LexerPopModeAction&) = default;

    LexerPopModeAction& operator=(LexerPopModeAction&&) = default;

    /// <summary>
    /// {@inheritDoc} </summary>
    /// <returns> This method returns <seealso cref="LexerActionType#MODE"/>. </returns>
    virtual LexerActionType getActionType() const override;

    /// <summary>
    /// {@inheritDoc} </summary>
    /// <returns> This method returns {@code false}. </returns>
    virtual bool isPositionDependent() const override;

    /// <summary>
    /// {@inheritDoc}
    ///
    /// <para>This action is implemented by calling <seealso cref="Lexer#popMode"/>.</para>
    /// </summary>
    virtual void execute(Lexer *lexer) const override;

    virtual size_t hashCode() const override;
    virtual bool equals(const LexerAction &obj) const override;
    virtual std::string toString() const override;
  };

} // namespace atn
} // namespace antlr4
