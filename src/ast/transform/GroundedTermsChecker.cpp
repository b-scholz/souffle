/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file GroundedTermsChecker.cpp
 *
 * Implementation of the grounded terms checker pass.
 *
 ***********************************************************************/

#include "ast/transform/GroundedTermsChecker.h"
#include "ast/Clause.h"
#include "ast/Program.h"
#include "ast/RecordInit.h"
#include "ast/TranslationUnit.h"
#include "ast/Variable.h"
#include "ast/analysis/Ground.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "reports/ErrorReport.h"
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace souffle {

void GroundedTermsChecker::verify(AstTranslationUnit& translationUnit) {
    auto&& program = *translationUnit.getProgram();
    auto&& report = translationUnit.getErrorReport();

    // -- check grounded variables and records --
    visitDepthFirst(program.getClauses(), [&](const AstClause& clause) {
        if (isFact(clause)) return;  // only interested in rules

        auto isGrounded = getGroundedTerms(translationUnit, clause);

        std::set<std::string> reportedVars;
        // all terms in head need to be grounded
        for (auto&& cur : getVariables(clause)) {
            if (!isGrounded[cur] && reportedVars.insert(cur->getName()).second) {
                report.addError("Ungrounded variable " + cur->getName(), cur->getSrcLoc());
            }
        }

        // all records need to be grounded
        visitDepthFirst(clause, [&](const AstRecordInit& record) {
            if (!isGrounded[&record]) {
                report.addError("Ungrounded record", record.getSrcLoc());
            }
        });

        // All sums need to be grounded
        visitDepthFirst(clause, [&](const AstBranchInit& adt) {
            if (!isGrounded[&adt]) {
                report.addError("Ungrounded ADT branch", adt.getSrcLoc());
            }
        });
    });
}

}  // end of namespace souffle
