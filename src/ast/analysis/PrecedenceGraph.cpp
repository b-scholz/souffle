/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file PrecedenceGraph.cpp
 *
 * Implements method of precedence graph to build the precedence graph,
 * compute strongly connected components of the precedence graph, and
 * build the strongly connected component graph.
 *
 ***********************************************************************/

#include "ast/analysis/PrecedenceGraph.h"
#include "GraphUtils.h"
#include "ast/Program.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/RelationDetailCache.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include <set>
#include <string>
#include <vector>

namespace souffle {

void PrecedenceGraphAnalysis::run(const AstTranslationUnit& translationUnit) {
    /* Get relations */
    const auto& program = *translationUnit.getProgram();
    const auto& relationDetail = *translationUnit.getAnalysis<RelationDetailCacheAnalysis>();

    for (const auto* r : program.getRelations()) {
        backingGraph.insert(r);
        for (const auto& c : relationDetail.getClauses(r)) {
            visitDepthFirst(c->getBodyLiterals(), [&](const AstAtom& atom) {
                backingGraph.insert(relationDetail.getRelation(atom.getQualifiedName()), r);
            });
            visitDepthFirst(c->getHead()->getArguments(), [&](const AstAtom& atom) {
                backingGraph.insert(relationDetail.getRelation(atom.getQualifiedName()), r);
            });
        }
    }
}

void PrecedenceGraphAnalysis::print(std::ostream& os) const {
    /* Print dependency graph */
    std::stringstream ss;
    ss << "digraph {\n";
    /* Print node of dependence graph */
    for (const AstRelation* rel : backingGraph.vertices()) {
        if (rel != nullptr) {
            ss << "\t\"" << rel->getQualifiedName() << "\" [label = \"" << rel->getQualifiedName()
               << "\"];\n";
        }
    }
    for (const AstRelation* rel : backingGraph.vertices()) {
        if (rel != nullptr) {
            for (const AstRelation* adjRel : backingGraph.successors(rel)) {
                if (adjRel != nullptr) {
                    ss << "\t\"" << rel->getQualifiedName() << "\" -> \"" << adjRel->getQualifiedName()
                       << "\";\n";
                }
            }
        }
    }
    ss << "}\n";
    printHTMLGraph(os, ss.str(), getName());
}

}  // end of namespace souffle
