/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RemoveEmptyRelations.cpp
 *
 ***********************************************************************/

#include "ast/transform/RemoveEmptyRelations.h"
#include "ast/Aggregator.h"
#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/Literal.h"
#include "ast/Negation.h"
#include "ast/Program.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/IOType.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace souffle {

bool RemoveEmptyRelationsTransformer::removeEmptyRelations(AstTranslationUnit& translationUnit) {
    AstProgram& program = *translationUnit.getProgram();
    auto* ioTypes = translationUnit.getAnalysis<IOType>();
    std::set<AstQualifiedName> emptyRelations;
    bool changed = false;
    for (auto rel : program.getRelations()) {
        if (!getClauses(program, *rel).empty() || ioTypes->isInput(rel)) {
            continue;
        }
        emptyRelations.insert(rel->getQualifiedName());

        bool usedInAggregate = false;
        visitDepthFirst(program, [&](const AstAggregator& agg) {
            for (const auto lit : agg.getBodyLiterals()) {
                visitDepthFirst(*lit, [&](const AstAtom& atom) {
                    if (getAtomRelation(&atom, &program) == rel) {
                        usedInAggregate = true;
                    }
                });
            }
        });

        if (!usedInAggregate && !ioTypes->isOutput(rel)) {
            removeRelation(translationUnit, rel->getQualifiedName());
            changed = true;
        }
    }

    for (const auto& relName : emptyRelations) {
        changed |= removeEmptyRelationUses(translationUnit, relName);
    }

    return changed;
}

bool RemoveEmptyRelationsTransformer::removeEmptyRelationUses(
        AstTranslationUnit& translationUnit, const AstQualifiedName& emptyRelationName) {
    AstProgram& program = *translationUnit.getProgram();
    bool changed = false;

    //
    // (1) drop rules from the program that have empty relations in their bodies.
    // (2) drop negations of empty relations
    //
    // get all clauses
    std::vector<const AstClause*> clauses;
    visitDepthFirst(program, [&](const AstClause& cur) { clauses.push_back(&cur); });

    // clean all clauses
    for (const AstClause* cl : clauses) {
        // check for an atom whose relation is the empty relation

        bool removed = false;
        for (AstLiteral* lit : cl->getBodyLiterals()) {
            if (auto* arg = dynamic_cast<AstAtom*>(lit)) {
                if (arg->getQualifiedName() == emptyRelationName) {
                    program.removeClause(cl);
                    removed = true;
                    changed = true;
                    break;
                }
            }
        }

        if (!removed) {
            // check whether a negation with empty relations exists

            bool rewrite = false;
            for (AstLiteral* lit : cl->getBodyLiterals()) {
                if (auto* neg = dynamic_cast<AstNegation*>(lit)) {
                    if (neg->getAtom()->getQualifiedName() == emptyRelationName) {
                        rewrite = true;
                        break;
                    }
                }
            }

            if (rewrite) {
                // clone clause without negation for empty relations

                auto res = std::unique_ptr<AstClause>(cloneHead(cl));

                for (AstLiteral* lit : cl->getBodyLiterals()) {
                    if (auto* neg = dynamic_cast<AstNegation*>(lit)) {
                        if (neg->getAtom()->getQualifiedName() != emptyRelationName) {
                            res->addToBody(souffle::clone(lit));
                        }
                    } else {
                        res->addToBody(souffle::clone(lit));
                    }
                }

                program.removeClause(cl);
                program.addClause(std::move(res));
                changed = true;
            }
        }
    }

    return changed;
}

}  // end of namespace souffle
