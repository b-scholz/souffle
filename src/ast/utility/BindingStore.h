/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file BindingStore.h
 *
 * Defines the BindingStore class, which can be used to dynamically
 * determine the set of bound variables within a given clause.
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"
#include <map>
#include <set>
#include <string>

namespace souffle {

class AstArgument;
class AstAtom;
class AstClause;

class BindingStore {
public:
    BindingStore(const AstClause* clause);

    /**
     * Mark the given variable as strongly bound.
     * Strongly bound variables can be used to bind functor arguments.
     * This is the usual case, e.g. body atom appearances
     */
    void bindVariableStrongly(std::string varName) {
        stronglyBoundVariables.insert(varName);

        // Some functor dependencies may be reduced
        reduceDependencies();
    }

    /**
     * Mark the given variable as weakly bound.
     * Weakly bound variables cannot be used to bind functor arguments.
     * E.g. bound head arguments in MST adorned relations
     */
    void bindVariableWeakly(std::string varName) {
        weaklyBoundVariables.insert(varName);
    }

    /** Check if a variable is bound */
    bool isBound(std::string varName) const {
        return contains(stronglyBoundVariables, varName) || contains(weaklyBoundVariables, varName);
    }

    /** Check if an argument is bound */
    bool isBound(const AstArgument* arg) const;

    /** Counts the number of bound arguments in the given atom */
    size_t numBoundArguments(const AstAtom* atom) const;

private:
    // Helper types to represent a disjunction of several dependency sets
    using ConjBindingSet = std::set<std::string>;
    using DisjBindingSet = std::set<ConjBindingSet>;

    std::set<std::string> stronglyBoundVariables{};
    std::set<std::string> weaklyBoundVariables{};
    std::map<std::string, DisjBindingSet> variableDependencies{};

    /**
     * Add a new conjunction of variables as a potential binder for a given variable.
     * The variable is considered bound if all variables in the conjunction are bound.
     */
    void addBindingDependency(std::string variable, ConjBindingSet dependency) {
        if (!contains(variableDependencies, variable)) {
            variableDependencies[variable] = DisjBindingSet();
        }
        variableDependencies[variable].insert(dependency);
    }

    /** Add binding dependencies formed on lhs by a <lhs> = <rhs> equality constraint. */
    void processEqualityBindings(const AstArgument* lhs, const AstArgument* rhs);

    /** Generate all binding dependencies implied by the constraints within a given clause. */
    void generateBindingDependencies(const AstClause* clause);

    /** Reduce a conjunctive set of dependencies based on the current bound variable set. */
    ConjBindingSet reduceDependency(const ConjBindingSet& origDependency);

    /** Reduce a disjunctive set of variable dependencies based on the current bound variable set. */
    DisjBindingSet reduceDependency(const DisjBindingSet& origDependency);

    /** Reduce the full set of dependencies for all tracked variables, binding whatever needs to be bound. */
    bool reduceDependencies();
};

}  // namespace souffle
