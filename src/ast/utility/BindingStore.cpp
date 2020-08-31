/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file BindingStore.cpp
 *
 * Defines the BindingStore class, which can be used to dynamically
 * determine the set of bound variables within a given clause.
 *
 ***********************************************************************/

#include "ast/utility/BindingStore.h"
#include "ast/Atom.h"
#include "ast/BinaryConstraint.h"
#include "ast/Clause.h"
#include "ast/Constant.h"
#include "ast/Variable.h"
#include "ast/utility/Visitor.h"

namespace souffle {

BindingStore::BindingStore(const AstClause* clause) {
    generateBindingDependencies(clause);
    reduceDependencies();
}

void BindingStore::generateBindingDependencies(const AstClause* clause) {
    // Grab all relevant constraints (i.e. eq. constrs not involving aggregators)
    std::set<const AstBinaryConstraint*> relevantEqConstraints;
    visitDepthFirst(*clause, [&](const AstBinaryConstraint& bc) {
        bool containsAggregators = false;
        visitDepthFirst(bc, [&](const AstAggregator& /* aggr */) { containsAggregators = true; });
        if (!containsAggregators && bc.getOperator() == BinaryConstraintOp::EQ) {
            relevantEqConstraints.insert(&bc);
        }
    });

    // Add variable binding dependencies implied by the constraint
    for (const auto* eqConstraint : relevantEqConstraints) {
        processEqualityBindings(eqConstraint->getLHS(), eqConstraint->getRHS());
        processEqualityBindings(eqConstraint->getRHS(), eqConstraint->getLHS());
    }
}

void BindingStore::processEqualityBindings(const AstArgument* lhs, const AstArgument* rhs) {
    // Only care about equalities affecting the bound status of variables
    const auto* var = dynamic_cast<const AstVariable*>(lhs);
    if (var == nullptr) return;

    // If all variables on the rhs are bound, then lhs is also bound
    BindingStore::ConjBindingSet depSet;
    visitDepthFirst(*rhs, [&](const AstVariable& subVar) { depSet.insert(subVar.getName()); });
    addBindingDependency(var->getName(), depSet);

    // If the lhs is bound, then all args in the rec on the rhs are also bound
    if (const auto* rec = dynamic_cast<const AstRecordInit*>(rhs)) {
        for (const auto* arg : rec->getArguments()) {
            const auto* subVar = dynamic_cast<const AstVariable*>(arg);
            assert(subVar != nullptr && "expected args to be variables");
            addBindingDependency(subVar->getName(), BindingStore::ConjBindingSet({var->getName()}));
        }
    }
}

BindingStore::ConjBindingSet BindingStore::reduceDependency(
        const BindingStore::ConjBindingSet& origDependency) {
    BindingStore::ConjBindingSet newDependency;
    for (const auto& var : origDependency) {
        // Only keep unbound variables in the dependency
        if (!contains(stronglyBoundVariables, var)) {
            newDependency.insert(var);
        }
    }
    return newDependency;
}

BindingStore::DisjBindingSet BindingStore::reduceDependency(
        const BindingStore::DisjBindingSet& origDependency) {
    BindingStore::DisjBindingSet newDependencies;
    for (const auto& dep : origDependency) {
        auto newDep = reduceDependency(dep);
        if (!newDep.empty()) {
            newDependencies.insert(newDep);
        }
    }
    return newDependencies;
}

bool BindingStore::reduceDependencies() {
    bool changed = false;
    std::map<std::string, BindingStore::DisjBindingSet> newVariableDependencies;
    std::set<std::string> variablesToBind;

    // Reduce each variable's set of dependencies one by one
    for (const auto& [headVar, dependencies] : variableDependencies) {
        // No need to track the dependencies of already-bound variables
        if (contains(stronglyBoundVariables, headVar)) {
            changed = true;
            continue;
        }

        // Reduce the dependency set based on bound variables
        auto newDependencies = reduceDependency(dependencies);
        if (newDependencies.empty() || newDependencies.size() < dependencies.size()) {
            // At least one dependency has been satisfied, so variable is now bound
            changed = true;
            variablesToBind.insert(headVar);
            continue;
        }
        newVariableDependencies[headVar] = newDependencies;
        changed |= (newDependencies != dependencies);
    }

    // Bind variables that need to be bound
    for (auto var : variablesToBind) {
        stronglyBoundVariables.insert(var);
    }

    // Repeat it recursively if any changes happened, until we reach a fixpoint
    if (changed) {
        variableDependencies = newVariableDependencies;
        reduceDependencies();
        return true;
    }
    assert(variableDependencies == newVariableDependencies && "unexpected change");
    return false;
}

bool BindingStore::isBound(const AstArgument* arg) const {
    if (const auto* var = dynamic_cast<const AstVariable*>(arg)) {
        return isBound(var->getName());
    } else if (const auto* term = dynamic_cast<const AstTerm*>(arg)) {
        for (const auto* subArg : term->getArguments()) {
            if (!isBound(subArg)) {
                return false;
            }
        }
        return true;
    } else if (dynamic_cast<const AstConstant*>(arg) != nullptr) {
        return true;
    } else {
        return false;
    }
}

size_t BindingStore::numBoundArguments(const AstAtom* atom) const {
    size_t count = 0;
    for (const auto* arg : atom->getArguments()) {
        if (isBound(arg)) {
            count++;
        }
    }
    return count;
}

}  // namespace souffle
