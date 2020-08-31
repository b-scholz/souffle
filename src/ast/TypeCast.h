/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TypeCast.h
 *
 * Defines the type cast class
 *
 ***********************************************************************/

#pragma once

#include "ast/Argument.h"
#include "ast/Node.h"
#include "ast/QualifiedName.h"
#include "ast/utility/NodeMapper.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstTypeCast
 * @brief Defines a type cast class for expressions
 */

class AstTypeCast : public AstArgument {
public:
    AstTypeCast(Own<AstArgument> value, AstQualifiedName type, SrcLocation loc = {})
            : AstArgument(std::move(loc)), value(std::move(value)), type(std::move(type)) {}

    /** Return value */
    AstArgument* getValue() const {
        return value.get();
    }

    /** Return cast type */
    const AstQualifiedName& getType() const {
        return type;
    }

    /** Set cast type */
    void setType(const AstQualifiedName& type) {
        this->type = type;
    }

    std::vector<const AstNode*> getChildNodes() const override {
        auto res = AstArgument::getChildNodes();
        res.push_back(value.get());
        return res;
    }

    AstTypeCast* clone() const override {
        return new AstTypeCast(souffle::clone(value), type, getSrcLoc());
    }

    void apply(const AstNodeMapper& map) override {
        value = map(std::move(value));
    }

protected:
    void print(std::ostream& os) const override {
        os << "as(" << *value << "," << type << ")";
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstTypeCast&>(node);
        return type == other.type && equal_ptr(value, other.value);
    }

    /** Casted value */
    Own<AstArgument> value;

    /** Cast type */
    AstQualifiedName type;
};

}  // end of namespace souffle
