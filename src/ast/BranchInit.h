/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All Rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file BranchInit.h
 *
 * Defines an argument covering the branch initialization of ADTs.
 *
 ***********************************************************************/

#pragma once

#include "ast/Argument.h"
#include "ast/Term.h"
#include "ast/utility/NodeMapper.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/tinyformat.h"
#include <cassert>
#include <memory>

namespace souffle {

/**
 * @class AstBranchInit
 * @brief Initialization of ADT instance.
 *
 * @param constructor An entity used to create a variant type. Can be though of as a name of the branch.
 *
 * Initializes one of the branches of ADT. The syntax for branches initialization is
 * $Constructor(args...)
 * In case of the branch with no arguments it is simplified to $Constructor.
 */
class AstBranchInit : public AstTerm {
public:
    AstBranchInit(std::string constructor, VecOwn<AstArgument> args, SrcLocation loc = {})
            : AstTerm(std::move(args), std::move(loc)), constructor(std::move(constructor)) {}

    const std::string& getConstructor() const {
        return constructor;
    }

    AstBranchInit* clone() const override {
        return new AstBranchInit(constructor, souffle::clone(args), getSrcLoc());
    }

protected:
    void print(std::ostream& os) const override {
        os << tfm::format("$%s(%s)", constructor, join(args, ", "));
    }

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        const auto& other = dynamic_cast<const AstBranchInit&>(node);
        return (constructor == other.constructor) && equal_targets(args, other.args);
    }

private:
    /** The adt branch constructor */
    std::string constructor;
};

}  // namespace souffle
