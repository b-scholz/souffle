/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SCCGraph.cpp
 *
 * Implements method of precedence graph to build the precedence graph,
 * compute strongly connected components of the precedence graph, and
 * build the strongly connected component graph.
 *
 ***********************************************************************/

#include "ast/analysis/AggAnalysis.h"
#include "ast/analysis/SCCGraph.h"
#include "Global.h"
#include "GraphUtils.h"
#include "ast/Program.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/Aggregator.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/IOType.h"
#include "ast/utility/Visitor.h"
#include "ast/utility/Utils.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <iostream>

namespace souffle::ast::analysis {

void AggAnalysis::run(const TranslationUnit& translationUnit) {
    auto sccGraph = translationUnit.getAnalysis<SCCGraphAnalysis>();
    Program& program = translationUnit.getProgram();
    std::set<const Relation *> relations; 

    // find relations that have at least one aggregate 
    // in their rules 
    for (const auto &clause : program.getClauses()) { 
       bool isAggregate = false; 
       visitDepthFirst(*clause, [&](const Aggregator& agg) {
          isAggregate = true; 
       }); 
       if (isAggregate) { 
        // std::cout << "Found relation with aggregate: " << clause->getHead()->getQualifiedName() << std::endl; 
        const Relation * rel = getRelation(program,clause->getHead()->getQualifiedName()); 
        relations.insert(rel); 
       } 
    } 

    // find SCC graph 
    std::set<int> worklist; 
    std::set<int> reachable; 

    // setup initial worklist 
    for(const auto rel: relations) { 
        int node = sccGraph->getSCC(rel); 
        // std::cout << "node: " << node << std::endl;
        worklist.insert(node); 
    } 
    // compute reachable 
    while(worklist.size() > 0) {
        // choose an scc node in the graph 
        auto node = *worklist.begin(); 
        worklist.erase(worklist.begin()); 
        // std::cout << "Select node: " << node << std::endl; 
        // add node to reachable scc set 
        reachable.insert(node); 
        // add predecessors 
        for(auto pred: sccGraph->getPredecessorSCCs(node)) { 
            // std::cout << "\tPred node: " << pred << std::endl; 
            if (reachable.count(pred) == 0) { 
                worklist.insert(pred);
            } 
        } 
    } 

    // find all relations 
    for(auto node: reachable) { 
        for(auto rel: sccGraph->getRelations(node)) { 
            relations.insert(rel); 
        } 
    } 

    // std::cout << "Full list:" << std::endl; 
    for(auto *rel: relations) { 
        std::cout << rel->getQualifiedName() << std::endl; 
    } 
}


}  // namespace souffle::ast::analysis
