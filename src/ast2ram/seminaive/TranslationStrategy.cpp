/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TranslationStrategy.cpp
 *
 ***********************************************************************/

#include "ast2ram/seminaive/TranslationStrategy.h"
#include "ast2ram/seminaive/ClauseTranslator.h"
#include "ast2ram/seminaive/ConstraintTranslator.h"
#include "ast2ram/seminaive/UnitTranslator.h"
#include "ast2ram/seminaive/ValueTranslator.h"
#include "ast2ram/utility/TranslatorContext.h"
#include "ram/Condition.h"
#include "ram/Expression.h"
#include "souffle/SymbolTable.h"

namespace souffle::ast2ram::seminaive {

ast2ram::UnitTranslator* TranslationStrategy::createUnitTranslator() const {
    return new UnitTranslator();
}

ast2ram::ClauseTranslator* TranslationStrategy::createClauseTranslator(
        const TranslatorContext& context, SymbolTable& symbolTable) const {
    return new ClauseTranslator(context, symbolTable);
}

ast2ram::ConstraintTranslator* TranslationStrategy::createConstraintTranslator(
        const TranslatorContext& context, SymbolTable& symbolTable, const ValueIndex& index) const {
    return new ConstraintTranslator(context, symbolTable, index);
}

ast2ram::ValueTranslator* TranslationStrategy::createValueTranslator(
        const TranslatorContext& context, SymbolTable& symbolTable, const ValueIndex& index) const {
    return new ValueTranslator(context, symbolTable, index);
}

}  // namespace souffle::ast2ram::seminaive
