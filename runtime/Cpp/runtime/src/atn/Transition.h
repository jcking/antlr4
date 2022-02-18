/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/TransitionType.h"
#include "misc/IntervalSet.h"

namespace antlr4 {
namespace atn {

  /// <summary>
  /// An ATN transition between any two ATN states.  Subclasses define
  ///  atom, set, epsilon, action, predicate, rule transitions.
  /// <p/>
  ///  This is a one way link.  It emanates from a state (usually via a list of
  ///  transitions) and has a target state.
  /// <p/>
  ///  Since we never have to change the ATN transitions once we construct it,
  ///  we can fix these transitions as specific classes. The DFA transitions
  ///  on the other hand need to update the labels as it adds transitions to
  ///  the states. We'll use the term Edge for the DFA to distinguish them from
  ///  ATN transitions.
  /// </summary>
  class ANTLR4CPP_PUBLIC Transition {
  public:
    Transition(const Transition&) = default;

    Transition(Transition&&) = default;

    virtual ~Transition() = default;

    Transition& operator=(const Transition&) = default;

    Transition& operator=(Transition&&) = default;

    void setTarget(ATNState* target);

    constexpr ATNState* getTarget() const { return _target; }

    virtual TransitionType getType() const = 0;

    /**
     * Determines if the transition is an "epsilon" transition.
     *
     * <p>The default implementation returns {@code false}.</p>
     *
     * @return {@code true} if traversing this transition in the ATN does not
     * consume an input symbol; otherwise, {@code false} if traversing this
     * transition consumes (matches) an input symbol.
     */
    virtual bool isEpsilon() const;
    virtual const misc::IntervalSet& label() const;
    virtual bool matches(size_t symbol, size_t minVocabSymbol, size_t maxVocabSymbol) const = 0;

    virtual bool equals(const Transition &other) const;

    virtual std::string toString() const;

  protected:
    explicit Transition(ATNState *target);

  private:
    /// The target of this transition.
    // ml: this is a reference into the ATN.
    ATNState *_target;
  };

  inline bool operator==(const Transition &lhs, const Transition &rhs) { return lhs.equals(rhs); }

  inline bool operator!=(const Transition &lhs, const Transition &rhs) { return !operator==(lhs, rhs); }

} // namespace atn
} // namespace antlr4
