/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include <cassert>

#include "support/BitSet.h"
#include "atn/PredictionContext.h"
#include "atn/ATNConfig.h"
#include "atn/AnySemanticContext.h"

namespace antlr4 {
namespace atn {

  /// Specialized set that can track info about the set, with support for combining similar configurations using a
  /// graph-structured stack.
  class ANTLR4CPP_PUBLIC ATNConfigSet {
  private:
    static size_t hashCode(bool ordered, const ATNConfig &atnConfig);

    static bool equals(bool ordered, const ATNConfig &lhs, const ATNConfig &rhs);

    struct ATNConfigHasher final {
      bool ordered;

      size_t operator()(const ATNConfig &other) const {
        return ATNConfigSet::hashCode(ordered, other);
      }
    };

    struct ATNConfigComparer final {
      bool ordered;

      bool operator()(const ATNConfig &lhs, const ATNConfig &rhs) const {
        return ATNConfigSet::equals(ordered, lhs, rhs);
      }
    };

    using Container = std::unordered_set<ATNConfig, ATNConfigHasher, ATNConfigComparer>;

  public:
    using value_type = ATNConfig;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = ATNConfig&;
    using const_reference = const ATNConfig&;
    using pointer = ATNConfig*;
    using const_pointer = const ATNConfig*;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // TODO: these fields make me pretty uncomfortable but nice to pack up info together, saves recomputation
    // TODO: can we track conflicts as they are added to save scanning configs later?
    size_t uniqueAlt = 0;

    /** Currently this is only used when we detect SLL conflict; this does
     *  not necessarily represent the ambiguous alternatives. In fact,
     *  I should also point out that this seems to include predicated alternatives
     *  that have predicates that evaluate to false. Computed in computeTargetState().
     */
    antlrcpp::BitSet conflictingAlts;

    // Used in parser and lexer. In lexer, it indicates we hit a pred
    // while computing a closure operation.  Don't make a DFA state from this.
    bool hasSemanticContext = false;
    bool dipsIntoOuterContext = false;

    /// Indicates that this configuration set is part of a full context
    /// LL prediction. It will be used to determine how to merge $. With SLL
    /// it's a wildcard whereas it is not for LL context merge.
    bool fullCtx = true;

    ATNConfigSet();

    explicit ATNConfigSet(bool fullCtx);

    ATNConfigSet(const ATNConfigSet&) = default;

    ATNConfigSet(ATNConfigSet&&) = default;

    ATNConfigSet& operator=(const ATNConfigSet&) = default;

    ATNConfigSet& operator=(ATNConfigSet&&) = default;

    /// <summary>
    /// Adding a new config means merging contexts with existing configs for
    /// {@code (s, i, pi, _)}, where {@code s} is the
    /// <seealso cref="ATNConfig#state"/>, {@code i} is the <seealso cref="ATNConfig#alt"/>, and
    /// {@code pi} is the <seealso cref="ATNConfig#semanticContext"/>. We use
    /// {@code (s,i,pi)} as key.
    /// <p/>
    /// This method updates <seealso cref="#dipsIntoOuterContext"/> and
    /// <seealso cref="#hasSemanticContext"/> when necessary.
    /// </summary>
    bool add(const ATNConfig& config);

    bool addAll(const ATNConfigSet &other);

    void reserve(size_t size);

    std::vector<ATNState *> getStates() const;

    /**
     * Gets the complete set of represented alternatives for the configuration
     * set.
     *
     * @return the set of represented alternatives in this configuration set
     *
     * @since 4.3
     */
    antlrcpp::BitSet getAlts() const;
    std::vector<AnySemanticContext> getPredicates() const;

    void optimizeConfigs(ATNSimulator *interpreter);

    iterator begin() { return _configLookup.begin(); }

    iterator end() { return _configLookup.end(); }

    const_iterator begin() const { return _configLookup.begin(); }

    const_iterator end() const { return _configLookup.end(); }

    const_iterator cbegin() const { return _configLookup.cbegin(); }

    const_iterator cend() const { return _configLookup.cend(); }

    size_t size() const;
    bool isEmpty() const;
    void clear();
    bool isReadonly() const;
    void setReadonly(bool readonly);

    size_t hashCode() const;

    bool equals(const ATNConfigSet &other) const;

    std::string toString() const;

    friend void swap(ATNConfigSet &lhs, ATNConfigSet &rhs) {
      std::swap(lhs.uniqueAlt, rhs.uniqueAlt);
      std::swap(lhs.conflictingAlts, rhs.conflictingAlts);
      std::swap(lhs.hasSemanticContext, rhs.hasSemanticContext);
      std::swap(lhs.dipsIntoOuterContext, rhs.dipsIntoOuterContext);
      std::swap(lhs.fullCtx, rhs.fullCtx);
      std::swap(lhs._cachedHashCode, rhs._cachedHashCode);
      std::swap(lhs._readonly, rhs._readonly);
      std::swap(lhs._configLookup, rhs._configLookup);
    }

  protected:
    ATNConfigSet(bool fullCtx, bool ordered);

  private:
    mutable size_t _cachedHashCode = 0;

    /// Indicates that the set of configurations is read-only. Do not
    /// allow any code to manipulate the set; DFA states will point at
    /// the sets and they must not change. This does not protect the other
    /// fields; in particular, conflictingAlts is set after
    /// we've made this readonly.
    bool _readonly = false;

    /// All configs but hashed by (s, i, _, pi) not including context. Wiped out
    /// when we go readonly as this set becomes a DFA state.
    Container _configLookup;
  };

  inline bool operator==(const ATNConfigSet &lhs, const ATNConfigSet &rhs) { return lhs.equals(rhs); }

  inline bool operator!=(const ATNConfigSet &lhs, const ATNConfigSet &rhs) { return !operator==(lhs, rhs); }

} // namespace atn
} // namespace antlr4

namespace std {

template <>
struct hash<::antlr4::atn::ATNConfigSet> {
  size_t operator()(const ::antlr4::atn::ATNConfigSet &atnConfigSet) const {
    return atnConfigSet.hashCode();
  }
};

} // namespace std
