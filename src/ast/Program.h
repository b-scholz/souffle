/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Program.h
 *
 * Defines the program class
 *
 ***********************************************************************/

#pragma once

#include "ast/Clause.h"
#include "ast/Component.h"
#include "ast/ComponentInit.h"
#include "ast/Directive.h"
#include "ast/FunctorDeclaration.h"
#include "ast/Node.h"
#include "ast/Pragma.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/Type.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <cassert>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

namespace souffle {

/**
 * @class AstProgram
 * @brief The program class consists of relations, clauses and types.
 */
class AstProgram : public AstNode {
public:
    /** Return types */
    std::vector<AstType*> getTypes() const {
        return toPtrVector(types);
    }

    /** Return relations */
    std::vector<AstRelation*> getRelations() const {
        return toPtrVector(relations);
    }

    /** Return clauses */
    std::vector<AstClause*> getClauses() const {
        return toPtrVector(clauses);
    }

    /** Return functor declarations */
    std::vector<AstFunctorDeclaration*> getFunctorDeclarations() const {
        return toPtrVector(functors);
    }

    /** Return relation directives */
    std::vector<AstDirective*> getDirectives() const {
        return toPtrVector(directives);
    }

    /** Add relation directive */
    void addDirective(Own<AstDirective> directive) {
        assert(directive && "NULL directive");
        directives.push_back(std::move(directive));
    }

    /** Return pragma directives */
    const VecOwn<AstPragma>& getPragmaDirectives() const {
        return pragmas;
    }

    /* Add relation */
    void addRelation(Own<AstRelation> relation) {
        assert(relation && "NULL relation");
        relations.push_back(std::move(relation));
    }

    /** Remove relation */
    bool removeRelationDecl(const AstQualifiedName& name) {
        for (auto it = relations.begin(); it != relations.end(); it++) {
            const auto& rel = *it;
            if (rel->getQualifiedName() == name) {
                relations.erase(it);
                return true;
            }
        }
        return false;
    }

    /** Set clauses */
    void setClauses(VecOwn<AstClause> newClauses) {
        clauses = std::move(newClauses);
    }

    /** Add a clause */
    void addClause(Own<AstClause> clause) {
        assert(clause != nullptr && "NULL clause");
        assert(clause->getHead() != nullptr && "Undefined head of the clause");
        clauses.push_back(std::move(clause));
    }

    /** Remove a clause */
    bool removeClause(const AstClause* clause) {
        assert(clause != nullptr && "NULL clause");
        for (auto it = clauses.begin(); it != clauses.end(); it++) {
            if (**it == *clause) {
                clauses.erase(it);
                return true;
            }
        }
        return false;
    }

    /** Remove a directive */
    bool removeDirective(const AstDirective* directive) {
        assert(directive != nullptr && "NULL directive");
        for (auto it = directives.begin(); it != directives.end(); it++) {
            if (**it == *directive) {
                directives.erase(it);
                return true;
            }
        }
        return false;
    }

    /** Return component declarations */
    std::vector<AstComponent*> getComponents() const {
        return toPtrVector(components);
    }

    /** Return component instantiations */
    std::vector<AstComponentInit*> getComponentInstantiations() const {
        return toPtrVector(instantiations);
    }

    AstProgram* clone() const override {
        auto res = new AstProgram();
        res->pragmas = souffle::clone(pragmas);
        res->components = souffle::clone(components);
        res->instantiations = souffle::clone(instantiations);
        res->types = souffle::clone(types);
        res->functors = souffle::clone(functors);
        res->relations = souffle::clone(relations);
        res->clauses = souffle::clone(clauses);
        res->directives = souffle::clone(directives);
        return res;
    }

    void apply(const AstNodeMapper& map) override {
        for (auto& cur : pragmas) {
            cur = map(std::move(cur));
        }
        for (auto& cur : components) {
            cur = map(std::move(cur));
        }
        for (auto& cur : instantiations) {
            cur = map(std::move(cur));
        }
        for (auto& cur : functors) {
            cur = map(std::move(cur));
        }
        for (auto& cur : types) {
            cur = map(std::move(cur));
        }
        for (auto& cur : relations) {
            cur = map(std::move(cur));
        }
        for (auto& cur : clauses) {
            cur = map(std::move(cur));
        }
        for (auto& cur : directives) {
            cur = map(std::move(cur));
        }
    }

    std::vector<const AstNode*> getChildNodes() const override {
        std::vector<const AstNode*> res;
        for (const auto& cur : pragmas) {
            res.push_back(cur.get());
        }
        for (const auto& cur : components) {
            res.push_back(cur.get());
        }
        for (const auto& cur : instantiations) {
            res.push_back(cur.get());
        }
        for (const auto& cur : functors) {
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
        auto show = [&](auto&& xs, char const* sep = "\n") {
            if (!xs.empty()) os << join(xs, sep) << "\n";
        };

        show(pragmas, "\n\n");
        show(components);
        show(instantiations);
        show(types);
        show(functors);
        show(relations);
        show(clauses, "\n\n");
        show(directives, "\n\n");
    }

    bool equal(const AstNode& node) const override {
        const auto& other = static_cast<const AstProgram&>(node);
        if (!equal_targets(pragmas, other.pragmas)) {
            return false;
        }
        if (!equal_targets(components, other.components)) {
            return false;
        }
        if (!equal_targets(instantiations, other.instantiations)) {
            return false;
        }
        if (!equal_targets(functors, other.functors)) {
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
        return true;
    }

protected:
    friend class ComponentInstantiationTransformer;
    friend class ParserDriver;

    /* Add type */
    void addType(Own<AstType> type) {
        assert(type != nullptr && "NULL type");
        types.push_back(std::move(type));
    }

    /** Add a pragma */
    void addPragma(Own<AstPragma> pragma) {
        assert(pragma && "NULL pragma");
        pragmas.push_back(std::move(pragma));
    }

    /** Add functor */
    void addFunctorDeclaration(Own<souffle::AstFunctorDeclaration> functor) {
        assert(functor && "NULL functor-declaration");
        functors.push_back(std::move(functor));
    }

    /** Add component */
    void addComponent(Own<AstComponent> component) {
        assert(component && "NULL component");
        components.push_back(std::move(component));
    }

    /** Add component instantiation */
    void addInstantiation(Own<AstComponentInit> instantiation) {
        assert(instantiation && "NULL instantiation");
        instantiations.push_back(std::move(instantiation));
    }

    /** Program types  */
    VecOwn<AstType> types;

    /** Program relations */
    VecOwn<AstRelation> relations;

    /** External Functors */
    VecOwn<AstFunctorDeclaration> functors;

    /** Program clauses */
    VecOwn<AstClause> clauses;

    /** Directives */
    VecOwn<AstDirective> directives;

    /** Component definitions */
    VecOwn<AstComponent> components;

    /** Component instantiations */
    VecOwn<AstComponentInit> instantiations;

    /** Pragmas */
    VecOwn<AstPragma> pragmas;
};

}  // namespace souffle
