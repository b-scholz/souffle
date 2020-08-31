/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Component.h
 *
 * Defines the component class
 *
 ***********************************************************************/

#pragma once

#include "ast/Clause.h"
#include "ast/ComponentInit.h"
#include "ast/ComponentType.h"
#include "ast/Directive.h"
#include "ast/Node.h"
#include "ast/Relation.h"
#include "ast/Type.h"
#include "ast/utility/NodeMapper.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstComponent
 * @brief Component class
 *
 * Example:
 *   .comp X = {
 *      .decl A(y:number)
 *      A(1).
 *   }
 *
 * Component consists of type declaration, relations, rules, etc.
 */
class AstComponent : public AstNode {
public:
    /** Get component type */
    const AstComponentType* getComponentType() const {
        return componentType.get();
    }

    /** Set component type */
    void setComponentType(Own<AstComponentType> other) {
        componentType = std::move(other);
    }

    /** Get base components */
    const std::vector<AstComponentType*> getBaseComponents() const {
        return toPtrVector(baseComponents);
    }

    /** Add base components */
    void addBaseComponent(Own<AstComponentType> component) {
        baseComponents.push_back(std::move(component));
    }

    /** Add type */
    void addType(Own<AstType> t) {
        types.push_back(std::move(t));
    }

    /** Get types */
    std::vector<AstType*> getTypes() const {
        return toPtrVector(types);
    }

    /** Copy base components */
    void copyBaseComponents(const AstComponent& other) {
        baseComponents = souffle::clone(other.baseComponents);
    }

    /** Add relation */
    void addRelation(Own<AstRelation> r) {
        relations.push_back(std::move(r));
    }

    /** Get relations */
    std::vector<AstRelation*> getRelations() const {
        return toPtrVector(relations);
    }

    /** Add clause */
    void addClause(Own<AstClause> c) {
        clauses.push_back(std::move(c));
    }

    /** Get clauses */
    std::vector<AstClause*> getClauses() const {
        return toPtrVector(clauses);
    }

    /** Add directive */
    void addDirective(Own<AstDirective> directive) {
        directives.push_back(std::move(directive));
    }

    /** Get directive statements */
    std::vector<AstDirective*> getDirectives() const {
        return toPtrVector(directives);
    }

    /** Add components */
    void addComponent(Own<AstComponent> c) {
        components.push_back(std::move(c));
    }

    /** Get components */
    std::vector<AstComponent*> getComponents() const {
        return toPtrVector(components);
    }

    /** Add instantiation */
    void addInstantiation(Own<AstComponentInit> i) {
        instantiations.push_back(std::move(i));
    }

    /** Get instantiation */
    std::vector<AstComponentInit*> getInstantiations() const {
        return toPtrVector(instantiations);
    }

    /** Add override */
    void addOverride(const std::string& name) {
        overrideRules.insert(name);
    }

    /** Get override */
    const std::set<std::string>& getOverridden() const {
        return overrideRules;
    }

    AstComponent* clone() const override {
        auto* res = new AstComponent();
        res->componentType = souffle::clone(componentType);
        res->baseComponents = souffle::clone(baseComponents);
        res->components = souffle::clone(components);
        res->instantiations = souffle::clone(instantiations);
        res->types = souffle::clone(types);
        res->relations = souffle::clone(relations);
        res->clauses = souffle::clone(clauses);
        res->directives = souffle::clone(directives);
        res->overrideRules = overrideRules;
        return res;
    }

    void apply(const AstNodeMapper& mapper) override {
        componentType = mapper(std::move(componentType));
        for (auto& cur : baseComponents) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : components) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : instantiations) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : types) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : relations) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : clauses) {
            cur = mapper(std::move(cur));
        }
        for (auto& cur : directives) {
            cur = mapper(std::move(cur));
        }
    }

    std::vector<const AstNode*> getChildNodes() const override {
        std::vector<const AstNode*> res;

        res.push_back(componentType.get());
        for (const auto& cur : baseComponents) {
            res.push_back(cur.get());
        }
        for (const auto& cur : components) {
            res.push_back(cur.get());
        }
        for (const auto& cur : instantiations) {
            res.push_back(cur.get());
        }
        for (const auto& cur : types) {
            res.push_back(cur.get());
        }
        for (const auto& cur : relations) {
            res.push_back(cur.get());
        }
        for (const auto& cur : clauses) {
            res.push_back(cur.get());
        }
        for (const auto& cur : directives) {
            res.push_back(cur.get());
        }
        return res;
    }

protected:
    void print(std::ostream& os) const override {
        auto show = [&](auto&& xs, char const* sep = "\n", char const* prefix = "") {
            if (xs.empty()) return;
            os << prefix << join(xs, sep) << "\n";
        };

        os << ".comp " << *componentType << " ";
        show(baseComponents, ",", ": ");
        os << "{\n";
        show(components);
        show(instantiations);
        show(types);
        show(relations);
        show(overrideRules, ",", ".override ");
        show(clauses, "\n\n");
        show(directives, "\n\n");
        os << "}\n";
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstComponent&>(node);

        if (equal_ptr(componentType, other.componentType)) {
            return true;
        }
        if (!equal_targets(baseComponents, other.baseComponents)) {
            return false;
        }
        if (!equal_targets(components, other.components)) {
            return false;
        }
        if (!equal_targets(instantiations, other.instantiations)) {
            return false;
        }
        if (!equal_targets(types, other.types)) {
            return false;
        }
        if (!equal_targets(relations, other.relations)) {
            return false;
        }
        if (!equal_targets(clauses, other.clauses)) {
            return false;
        }
        if (!equal_targets(directives, other.directives)) {
            return false;
        }
        if (overrideRules != other.overrideRules) {
            return false;
        }
        return true;
    }

    /** Name of component and its formal component arguments. */
    Own<AstComponentType> componentType;

    /** Base components of component */
    VecOwn<AstComponentType> baseComponents;

    /** Types declarations */
    VecOwn<AstType> types;

    /** Relations */
    VecOwn<AstRelation> relations;

    /** Clauses */
    VecOwn<AstClause> clauses;

    /** I/O directives */
    VecOwn<AstDirective> directives;

    /** Nested components */
    VecOwn<AstComponent> components;

    /** Nested component instantiations. */
    VecOwn<AstComponentInit> instantiations;

    /** Clauses of relations that are overwritten by this component */
    std::set<std::string> overrideRules;
};

}  // end of namespace souffle
