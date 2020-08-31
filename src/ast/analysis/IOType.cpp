/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file IOType.cpp
 *
 * Implements methods to identify a relation as input, output, or printsize.
 *
 ***********************************************************************/

#include "ast/analysis/IOType.h"
#include "ast/Directive.h"
#include "ast/Program.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "souffle/utility/StreamUtil.h"
#include <ostream>
#include <vector>

namespace souffle {

void IOType::run(const AstTranslationUnit& translationUnit) {
    const AstProgram& program = *translationUnit.getProgram();
    visitDepthFirst(program, [&](const AstDirective& directive) {
        auto* relation = getRelation(program, directive.getQualifiedName());
        if (relation == nullptr) {
            return;
        }
        switch (directive.getType()) {
            case AstDirectiveType::input: inputRelations.insert(relation); break;
            case AstDirectiveType::output: outputRelations.insert(relation); break;
            case AstDirectiveType::printsize:
                printSizeRelations.insert(relation);
                outputRelations.insert(relation);
                break;
            case AstDirectiveType::limitsize:
                limitSizeRelations.insert(relation);
                assert(directive.hasDirective("n") && "limitsize has no n directive");
                limitSize[relation] = stoi(directive.getDirective("n"));
                break;
        }
    });
}

void IOType::print(std::ostream& os) const {
    auto show = [](std::ostream& os, const AstRelation* r) { os << r->getQualifiedName(); };
    os << "input relations: {" << join(inputRelations, ", ", show) << "}\n";
    os << "output relations: {" << join(outputRelations, ", ", show) << "}\n";
    os << "printsize relations: {" << join(printSizeRelations, ", ", show) << "}\n";
    os << "limitsize relations: {" << join(limitSizeRelations, ", ", show) << "}\n";
}

}  // end of namespace souffle
