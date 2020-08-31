/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Term.h
 *
 * Defines the abstract term class
 *
 ***********************************************************************/

#pragma once

#include "ast/Argument.h"
#include "ast/Node.h"
#include "ast/utility/NodeMapper.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/ContainerUtil.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstTerm
 * @brief Defines an abstract term class used for functors and other constructors
 */
class AstTerm : public AstArgument {
protected:
    template <typename... Operands>
    AstTerm(Operands&&... operands) : AstTerm(asVec(std::forward<Operands>(operands)...)) {}

    template <typename... Operands>
    AstTerm(SrcLocation loc, Operands&&... operands)
            : AstTerm(asVec(std::forward<Operands>(operands)...), std::move(loc)) {}

    AstTerm(VecOwn<AstArgument> operands, SrcLocation loc = {})
            : AstArgument(std::move(loc)), args(std::move(operands)) {}

public:
    /** Get arguments */
    std::vector<AstArgument*> getArguments() const {
        return toPtrVector(args);
    }

    /** Add argument to argument list */
    void addArgument(Own<AstArgument> arg) {
        args.push_back(std::move(arg));
    }

    std::vector<const AstNode*> getChildNodes() const override {
        auto res = AstArgument::getChildNodes();
        for (auto& cur : args) {
            res.push_back(cur.get());
        }
        return res;
    }

    void apply(const AstNodeMapper& map) override {
        for (auto& arg : args) {
            arg = map(std::move(arg));
        }
    }

protected:
    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstTerm&>(node);
        return equal_targets(args, other.args);
    }

    /** Arguments */
    VecOwn<AstArgument> args;

private:
    template <typename... Operands>
    static VecOwn<AstArgument> asVec(Operands... ops) {
        Own<AstArgument> ary[] = {std::move(ops)...};
        VecOwn<AstArgument> xs;
        for (auto&& x : ary) {
            xs.push_back(std::move(x));
        }
        return xs;
    }
};

}  // end of namespace souffle
