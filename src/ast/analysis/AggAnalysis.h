/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file AggAnalysis.h
 *
 ***********************************************************************/

#pragma once

#include "ast/TranslationUnit.h"
#include "ast/analysis/Analysis.h"
#include "ast/analysis/SCCGraph.h"

namespace souffle::ast {

class TranslationUnit;

namespace analysis {

/**
 * Analysis pass
 */
class AggAnalysis : public Analysis {
public:
    static constexpr const char* name = "agg-graph";

    AggAnalysis() : Analysis(name) {}

    void run(const TranslationUnit& translationUnit) override;

};

}  // namespace analysis
}  // namespace souffle::ast
