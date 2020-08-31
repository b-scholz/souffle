/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Clause.h
 *
 * Defines the clause class
 *
 ***********************************************************************/

#pragma once

#include "ast/Atom.h"
#include "ast/ExecutionPlan.h"
#include "ast/Literal.h"
#include "ast/Node.h"
#include "ast/utility/NodeMapper.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstClause
 * @brief Intermediate representation of a horn clause
 *
 *  A clause can either be:
 *      - a fact  - a clause with no body (e.g., X(a,b))
 *      - a rule  - a clause with a head and a body (e.g., Y(a,b) -: X(a,b))
 */
class AstClause : public AstNode {
public:
    AstClause(Own<AstAtom> head = {}, VecOwn<AstLiteral> bodyLiterals = {}, Own<AstExecutionPlan> plan = {},
            SrcLocation loc = {})
            : AstNode(std::move(loc)), head(std::move(head)), bodyLiterals(std::move(bodyLiterals)),
              plan(std::move(plan)) {}

    /** Add a literal to the body of the clause */
    void addToBody(Own<AstLiteral> literal) {
        bodyLiterals.push_back(std::move(literal));
    }

    /** Set the head of clause to @p h */
    void setHead(Own<AstAtom> h) {
        head = std::move(h);
    }

    /** Set the bodyLiterals of clause to @p body */
    void setBodyLiterals(VecOwn<AstLiteral> body) {
        bodyLiterals = std::move(body);
    }

    /** Return the atom that represents the head of the clause */
    AstAtom* getHead() const {
        return head.get();
    }

    /** Obtains a copy of the internally maintained body literals */
    std::vector<AstLiteral*> getBodyLiterals() const {
        return toPtrVector(bodyLiterals);
    }

    /** Obtains the execution plan associated to this clause or null if there is none */
    const AstExecutionPlan* getExecutionPlan() const {
        return plan.get();
    }

    /** Updates the execution plan associated to this clause */
    void setExecutionPlan(Own<AstExecutionPlan> plan) {
        this->plan = std::move(plan);
    }

    /** Resets the execution plan */
    void clearExecutionPlan() {
        plan = nullptr;
    }

    AstClause* clone() const override {
        return new AstClause(
                souffle::clone(head), souffle::clone(bodyLiterals), souffle::clone(plan), getSrcLoc());
    }

    void apply(const AstNodeMapper& map) override {
        head = map(std::move(head));
        for (auto& lit : bodyLiterals) {
            lit = map(std::move(lit));
        }
    }

    std::vector<const AstNode*> getChildNodes() const override {
        std::vector<const AstNode*> res = {head.get()};
        for (auto& cur : bodyLiterals) {
            res.push_back(cur.get());
        }
        return res;
    }

protected:
    void print(std::ostream& os) const override {
        if (head != nullptr) {
            os << *head;
        }
        if (!bodyLiterals.empty()) {
            os << " :- \n   " << join(bodyLiterals, ",\n   ");
        }
        os << ".";
        if (plan != nullptr) {
            os << *plan;
        }
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstClause&>(node);
        return equal_ptr(head, other.head) && equal_targets(bodyLiterals, other.bodyLiterals) &&
               equal_ptr(plan, other.plan);
    }

    /** Head of the clause */
    Own<AstAtom> head;

    /** Body literals of clause */
    VecOwn<AstLiteral> bodyLiterals;

    /** User defined execution plan (if not defined, plan is null) */
    Own<AstExecutionPlan> plan;
};

}  // end of namespace souffle
