/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "Recognizer.h"
#include "support/CPPUtils.h"
#include "atn/SemanticContextType.h"

namespace antlr4 {
namespace atn {

  class AnySemanticContext;

  /// A tree structure used to record the semantic context in which
  ///  an ATN configuration is valid.  It's either a single predicate,
  ///  a conjunction "p1 && p2", or a sum of products "p1||p2".
  ///
  ///  I have scoped the AND, OR, and Predicate subclasses of
  ///  SemanticContext within the scope of this outer class.
  class ANTLR4CPP_PUBLIC SemanticContext {
  public:
    /**
     * The default {@link SemanticContext}, which is semantically equivalent to
     * a predicate of the form {@code {true}?}.
     */
    static AnySemanticContext none();

    static AnySemanticContext And(AnySemanticContext lhs, AnySemanticContext rhs);

    static AnySemanticContext Or(AnySemanticContext lhs, AnySemanticContext rhs);

    SemanticContext(const SemanticContext&) = default;

    SemanticContext(SemanticContext&&) = default;

    virtual ~SemanticContext() = default;

    SemanticContext& operator=(const SemanticContext&) = default;

    SemanticContext& operator=(SemanticContext&&) = default;

    virtual SemanticContextType getType() const = 0;

    /// <summary>
    /// For context independent predicates, we evaluate them without a local
    /// context (i.e., null context). That way, we can evaluate them without
    /// having to create proper rule-specific context during prediction (as
    /// opposed to the parser, which creates them naturally). In a practical
    /// sense, this avoids a cast exception from RuleContext to myruleContext.
    /// <p/>
    /// For context dependent predicates, we must pass in a local context so that
    /// references such as $arg evaluate properly as _localctx.arg. We only
    /// capture context dependent predicates in the context in which we begin
    /// prediction, so we passed in the outer context here in case of context
    /// dependent predicate evaluation.
    /// </summary>
    virtual bool eval(Recognizer *parser, RuleContext *parserCallStack) const = 0;

    /**
     * Evaluate the precedence predicates for the context and reduce the result.
     *
     * @param parser The parser instance.
     * @param parserCallStack
     * @return The simplified semantic context after precedence predicates are
     * evaluated, which will be one of the following values.
     * <ul>
     * <li>{@link #NONE}: if the predicate simplifies to {@code true} after
     * precedence predicates are evaluated.</li>
     * <li>{@code null}: if the predicate simplifies to {@code false} after
     * precedence predicates are evaluated.</li>
     * <li>{@code this}: if the semantic context is not changed as a result of
     * precedence predicate evaluation.</li>
     * <li>A non-{@code null} {@link SemanticContext}: the new simplified
     * semantic context after precedence predicates are evaluated.</li>
     * </ul>
     */
    virtual AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const = 0;

    virtual size_t hashCode() const = 0;

    virtual bool equals(const SemanticContext &other) const = 0;

    virtual std::string toString() const = 0;

    class Predicate;
    class PrecedencePredicate;
    class Operator;
    class AND;
    class OR;

  protected:
    SemanticContext() = default;
  };

  inline bool operator==(const SemanticContext &lhs, const SemanticContext &rhs) { return lhs.equals(rhs); }

  inline bool operator!=(const SemanticContext &lhs, const SemanticContext &rhs) { return !operator==(lhs, rhs); }

  class ANTLR4CPP_PUBLIC SemanticContext::Predicate final : public SemanticContext {
  public:
    Predicate(size_t ruleIndex, size_t predIndex, bool isCtxDependent);

    Predicate(const Predicate&) = default;

    Predicate(Predicate&&) = default;

    Predicate& operator=(const Predicate&) = default;

    Predicate& operator=(Predicate&&) = default;

    constexpr size_t getRuleIndex() const { return _ruleIndex; }

    constexpr size_t getPredIndex() const { return _predIndex; }

    constexpr bool isCtxDependent() const { return _isCtxDependent; }

    SemanticContextType getType() const override;

    virtual bool eval(Recognizer *parser, RuleContext *parserCallStack) const override;

    AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual size_t hashCode() const override;

    virtual bool equals(const SemanticContext &other) const override;

    virtual std::string toString() const override;

  private:
    size_t _ruleIndex;
    size_t _predIndex;
    bool _isCtxDependent;
  };

  class ANTLR4CPP_PUBLIC SemanticContext::PrecedencePredicate final : public SemanticContext {
  public:
    explicit PrecedencePredicate(int precedence);

    PrecedencePredicate(const PrecedencePredicate&) = default;

    PrecedencePredicate(PrecedencePredicate&&) = default;

    PrecedencePredicate& operator=(const PrecedencePredicate&) = default;

    PrecedencePredicate& operator=(PrecedencePredicate&&) = default;

    constexpr int getPrecedence() const { return _precedence; }

    SemanticContextType getType() const override;

    virtual bool eval(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual size_t hashCode() const override;

    virtual bool equals(const SemanticContext &other) const override;

    virtual std::string toString() const override;

  private:
    int _precedence;
  };

  inline bool operator<(const SemanticContext::PrecedencePredicate &lhs, const SemanticContext::PrecedencePredicate &rhs) {
    return lhs.getPrecedence() < rhs.getPrecedence();
  }

  /**
   * This is the base class for semantic context "operators", which operate on
   * a collection of semantic context "operands".
   *
   * @since 4.3
   */
  class ANTLR4CPP_PUBLIC SemanticContext::Operator : public SemanticContext {
  public:
    /**
     * Gets the operands for the semantic context operator.
     *
     * @return a collection of {@link SemanticContext} operands for the
     * operator.
     *
     * @since 4.3
     */

    virtual const std::vector<AnySemanticContext>& getOperands() const = 0;
  };

  /**
   * A semantic context which is true whenever none of the contained contexts
   * is false.
   */
  class ANTLR4CPP_PUBLIC SemanticContext::AND final : public SemanticContext::Operator {
  public:
    AND(AnySemanticContext lhs, AnySemanticContext rhs);

    AND(const AND&) = default;

    AND(AND&&) = default;

    AND& operator=(const AND&) = default;

    AND& operator=(AND&&) = default;

    virtual const std::vector<AnySemanticContext>& getOperands() const override;

    SemanticContextType getType() const override;
    /**
     * The evaluation of predicates by this context is short-circuiting, but
     * unordered.</p>
     */
    virtual bool eval(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual size_t hashCode() const override;

    virtual bool equals(const SemanticContext &other) const override;

    virtual std::string toString() const override;

  private:
    std::shared_ptr<std::vector<AnySemanticContext>> _operands;
  };

  /**
   * A semantic context which is true whenever at least one of the contained
   * contexts is true.
   */
  class ANTLR4CPP_PUBLIC SemanticContext::OR final : public SemanticContext::Operator {
  public:
    OR(AnySemanticContext lhs, AnySemanticContext rhs);

    OR(const OR&) = default;

    OR(OR&&) = default;

    OR& operator=(const OR&) = default;

    OR& operator=(OR&&) = default;

    virtual const std::vector<AnySemanticContext>& getOperands() const override;

    SemanticContextType getType() const override;

    /**
     * The evaluation of predicates by this context is short-circuiting, but
     * unordered.
     */
    virtual bool eval(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual AnySemanticContext evalPrecedence(Recognizer *parser, RuleContext *parserCallStack) const override;

    virtual size_t hashCode() const override;

    virtual bool equals(const SemanticContext &other) const override;

    virtual std::string toString() const override;

  private:
    std::shared_ptr<std::vector<AnySemanticContext>> _operands;
  };

}  // namespace atn
}  // namespace antlr4

// Hash function for SemanticContext, used in the MurmurHash::update function

namespace std {

  template <>
  struct hash<::antlr4::atn::SemanticContext> {
    size_t operator()(const ::antlr4::atn::SemanticContext &semanticContext) const {
      return semanticContext.hashCode();
    }
  };

}  // namespace std
