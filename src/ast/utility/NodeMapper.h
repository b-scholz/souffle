/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file NodeMapper.h
 *
 * Defines the node mapper class
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"
#include <cassert>
#include <memory>

namespace souffle {
class AstNode;

/**
 * An abstract class for manipulating AST Nodes by substitution
 */
class AstNodeMapper {
public:
    virtual ~AstNodeMapper() = default;

    /**
     * Abstract replacement method for a node.
     *
     * If the given nodes is to be replaced, the handed in node
     * will be destroyed by the mapper and the returned node
     * will become owned by the caller.
     */
    virtual Own<AstNode> operator()(Own<AstNode> node) const = 0;

    /**
     * Wrapper for any subclass of the AST node hierarchy performing type casts.
     */
    template <typename T>
    Own<T> operator()(Own<T> node) const {
        Own<AstNode> resPtr = (*this)(Own<AstNode>(static_cast<AstNode*>(node.release())));
        assert(nullptr != dynamic_cast<T*>(resPtr.get()) && "Invalid target node!");
        return Own<T>(dynamic_cast<T*>(resPtr.release()));
    }
};

}  // end of namespace souffle
