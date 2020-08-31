/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExecutionPlanChecker.cpp
 *
 * Implementation of the execution plan checker pass.
 *
 ***********************************************************************/

#include "ast/transform/ExecutionPlanChecker.h"
#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/ExecutionOrder.h"
#include "ast/ExecutionPlan.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/RecursiveClauses.h"
#include "ast/analysis/RelationSchedule.h"
#include "ast/utility/Utils.h"
#include "reports/ErrorReport.h"
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace souffle {
class AstRelation;

bool AstExecutionPlanChecker::transform(AstTranslationUnit& translationUnit) {
    auto* relationSchedule = translationUnit.getAnalysis<RelationScheduleAnalysis>();
    auto* recursiveClauses = translationUnit.getAnalysis<RecursiveClausesAnalysis>();
    auto&& report = translationUnit.getErrorReport();

    for (const RelationScheduleAnalysisStep& step : relationSchedule->schedule()) {
        const std::set<const AstRelation*>& scc = step.computed();
        for (const AstRelation* rel : scc) {
            for (const AstClause* clause : getClauses(*translationUnit.getProgram(), *rel)) {
                if (!recursiveClauses->recursive(clause)) {
                    continue;
                }
                if (clause->getExecutionPlan() == nullptr) {
                    continue;
                }
                int version = 0;
                for (const auto* atom : getBodyLiterals<AstAtom>(*clause)) {
                    if (scc.count(getAtomRelation(atom, translationUnit.getProgram())) != 0u) {
                        version++;
                    }
                }
                int maxVersion = -1;
                for (auto const& cur : clause->getExecutionPlan()->getOrders()) {
                    maxVersion = std::max(cur.first, maxVersion);
                }

                if (version <= maxVersion) {
                    for (const auto& cur : clause->getExecutionPlan()->getOrders()) {
                        if (cur.first >= version) {
                            report.addDiagnostic(Diagnostic(Diagnostic::Type::ERROR,
                                    DiagnosticMessage(
                                            "execution plan for version " + std::to_string(cur.first),
                                            cur.second->getSrcLoc()),
                                    {DiagnosticMessage("only versions 0.." + std::to_string(version - 1) +
                                                       " permitted")}));
                        }
                    }
                }
            }
        }
    }
    return false;
}

}  // end of namespace souffle
