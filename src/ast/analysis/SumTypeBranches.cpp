/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SumTypeBranches.cpp
 *
 * Calculate branch to types mapping.
 *
 ***********************************************************************/

#include "ast/analysis/SumTypeBranches.h"
#include "ast/AlgebraicDataType.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/TypeEnvironment.h"
#include "ast/analysis/TypeSystem.h"
#include "ast/utility/Visitor.h"

namespace souffle {

void SumTypeBranchesAnalysis::run(const AstTranslationUnit& tu) {
    const TypeEnvironment& env = tu.getAnalysis<TypeEnvironmentAnalysis>()->getTypeEnvironment();

    visitDepthFirst(tu.getProgram()->getTypes(), [&](const AstAlgebraicDataType& adt) {
        auto typeName = adt.getQualifiedName();
        if (!env.isType(typeName)) return;

        for (auto& branch : adt.getBranches()) {
            branchToType[branch->getConstructor()] = &env.getType(typeName);
        }
    });
}

}  // namespace souffle
