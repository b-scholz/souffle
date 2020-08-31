/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ComponentInit.h
 *
 * Defines the component-initialization class
 *
 ***********************************************************************/

#pragma once

#include "ast/ComponentType.h"
#include "ast/Node.h"
#include "ast/utility/NodeMapper.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/MiscUtil.h"
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstComponentInit
 * @brief Component initialization class
 *
 * Example:
 *  .init X=B<T1,T2>
 *
 * Intialization of a component with type parameters
 */
class AstComponentInit : public AstNode {
public:
    AstComponentInit(std::string name, Own<AstComponentType> type, SrcLocation loc = {})
            : AstNode(std::move(loc)), instanceName(std::move(name)), componentType(std::move(type)) {}

    /** Return instance name */
    const std::string& getInstanceName() const {
        return instanceName;
    }

    /** Set instance name */
    void setInstanceName(std::string name) {
        instanceName = std::move(name);
    }

    /** Return component type */
    const AstComponentType* getComponentType() const {
        return componentType.get();
    }

    /** Set component type */
    void setComponentType(Own<AstComponentType> type) {
        componentType = std::move(type);
    }

    AstComponentInit* clone() const override {
        return new AstComponentInit(instanceName, souffle::clone(componentType), getSrcLoc());
    }

    void apply(const AstNodeMapper& mapper) override {
        componentType = mapper(std::move(componentType));
    }

    std::vector<const AstNode*> getChildNodes() const override {
        return {componentType.get()};
    }

protected:
    void print(std::ostream& os) const override {
        os << ".init " << instanceName << " = " << *componentType;
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstComponentInit&>(node);
        return instanceName == other.instanceName && *componentType == *other.componentType;
    }

    /** Instance name */
    std::string instanceName;

    /** Actual component arguments for instantiation */
    Own<AstComponentType> componentType;
};

}  // end of namespace souffle
