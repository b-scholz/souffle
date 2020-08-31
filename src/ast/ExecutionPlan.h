/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExecutionPlan.h
 *
 * Defines an execution plan class
 *
 ***********************************************************************/

#pragma once

#include "ast/ExecutionOrder.h"
#include "ast/Node.h"
#include "ast/utility/NodeMapper.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @brief AstExecutionPlan
 * @class Defines a user-defined execution plan for a clause.
 *
 * An user-defined execution plan consists of one or more
 * execution orders. An execution order is a permutation
 * of atoms in a clause.
 *
 * Example:
 *   .plan 0:(1,2,3), 2:(3,2,1)
 *
 */
class AstExecutionPlan : public AstNode {
public:
    /** Set execution order for a given rule version */
    void setOrderFor(int version, Own<AstExecutionOrder> plan) {
        plans[version] = std::move(plan);
    }

    /** Get orders */
    std::map<int, const AstExecutionOrder*> getOrders() const {
        std::map<int, const AstExecutionOrder*> result;
        for (auto& plan : plans) {
            result.insert(std::make_pair(plan.first, plan.second.get()));
        }
        return result;
    }

    AstExecutionPlan* clone() const override {
        auto res = new AstExecutionPlan();
        res->setSrcLoc(getSrcLoc());
        for (auto& plan : plans) {
            res->setOrderFor(plan.first, Own<AstExecutionOrder>(plan.second->clone()));
        }
        return res;
    }

    void apply(const AstNodeMapper& map) override {
        for (auto& plan : plans) {
            plan.second = map(std::move(plan.second));
        }
    }

    std::vector<const AstNode*> getChildNodes() const override {
        std::vector<const AstNode*> childNodes;
        for (auto& plan : plans) {
            childNodes.push_back(plan.second.get());
        }
        return childNodes;
    }

protected:
    void print(std::ostream& out) const override {
        if (!plans.empty()) {
            out << " .plan ";
            out << join(plans, ", ",
                    [](std::ostream& os, const auto& arg) { os << arg.first << ":" << *arg.second; });
        }
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstExecutionPlan&>(node);
        return equal_targets(plans, other.plans);
    }

private:
    /** Mapping versions of clauses to execution orders */
    std::map<int, Own<AstExecutionOrder>> plans;
};

}  // end of namespace souffle
