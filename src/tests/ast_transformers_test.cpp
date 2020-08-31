/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ast_transformers_test.cpp
 *
 * Tests souffle's AST transformers.
 *
 ***********************************************************************/

#include "tests/test.h"

#include "ast/Clause.h"
#include "ast/Node.h"
#include "ast/Program.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/ClauseNormalisation.h"
#include "ast/transform/MagicSet.h"
#include "ast/transform/MinimiseProgram.h"
#include "ast/transform/RemoveRedundantRelations.h"
#include "ast/transform/RemoveRelationCopies.h"
#include "ast/transform/ResolveAliases.h"
#include "ast/utility/Utils.h"
#include "parser/ParserDriver.h"
#include "reports/DebugReport.h"
#include "reports/ErrorReport.h"
#include "souffle/utility/StringUtil.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace souffle {

namespace test {

TEST(AstTransformers, GroundTermPropagation) {
    ErrorReport errorReport;
    DebugReport debugReport;
    // load some test program
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .type D <: symbol
                .decl p(a:D,b:D)

                p(a,b) :- p(x,y), r = [x,y], s = r, s = [w,v], [w,v] = [a,b].
            )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    // check types in clauses
    AstClause* a = getClauses(program, "p")[0];

    EXPECT_EQ("p(a,b) :- \n   p(x,y),\n   r = [x,y],\n   s = r,\n   s = [w,v],\n   [w,v] = [a,b].",
            toString(*a));

    std::unique_ptr<AstClause> res = ResolveAliasesTransformer::resolveAliases(*a);
    std::unique_ptr<AstClause> cleaned = ResolveAliasesTransformer::removeTrivialEquality(*res);

    EXPECT_EQ(
            "p(x,y) :- \n   p(x,y),\n   [x,y] = [x,y],\n   [x,y] = [x,y],\n   [x,y] = [x,y],\n   [x,y] = "
            "[x,y].",
            toString(*res));
    EXPECT_EQ("p(x,y) :- \n   p(x,y).", toString(*cleaned));
}

TEST(AstTransformers, GroundTermPropagation2) {
    ErrorReport errorReport;
    DebugReport debugReport;
    // load some test program
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
               .type D <: symbol
               .decl p(a:D,b:D)

               p(a,b) :- p(x,y), x = y, x = a, y = b.
           )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    // check types in clauses
    AstClause* a = getClauses(program, "p")[0];

    EXPECT_EQ("p(a,b) :- \n   p(x,y),\n   x = y,\n   x = a,\n   y = b.", toString(*a));

    std::unique_ptr<AstClause> res = ResolveAliasesTransformer::resolveAliases(*a);
    std::unique_ptr<AstClause> cleaned = ResolveAliasesTransformer::removeTrivialEquality(*res);

    EXPECT_EQ("p(b,b) :- \n   p(b,b),\n   b = b,\n   b = b,\n   b = b.", toString(*res));
    EXPECT_EQ("p(b,b) :- \n   p(b,b).", toString(*cleaned));
}

TEST(AstTransformers, ResolveGroundedAliases) {
    // load some test program
    ErrorReport errorReport;
    DebugReport debugReport;
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .type D <: symbol
                .decl p(a:D,b:D)

                p(a,b) :- p(x,y), r = [x,y], s = r, s = [w,v], [w,v] = [a,b].
            )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    EXPECT_EQ("p(a,b) :- \n   p(x,y),\n   r = [x,y],\n   s = r,\n   s = [w,v],\n   [w,v] = [a,b].",
            toString(*getClauses(program, "p")[0]));

    std::make_unique<ResolveAliasesTransformer>()->apply(*tu);

    EXPECT_EQ("p(x,y) :- \n   p(x,y).", toString(*getClauses(program, "p")[0]));
}

TEST(AstTransformers, ResolveAliasesWithTermsInAtoms) {
    // load some test program
    ErrorReport errorReport;
    DebugReport debugReport;
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .type D <: symbol
                .decl p(a:D,b:D)

                p(x,c) :- p(x,b), p(b,c), c = b+1, x=c+2.
            )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    EXPECT_EQ("p(x,c) :- \n   p(x,b),\n   p(b,c),\n   c = (b+1),\n   x = (c+2).",
            toString(*getClauses(program, "p")[0]));

    std::make_unique<ResolveAliasesTransformer>()->apply(*tu);

    EXPECT_EQ("p(x,c) :- \n   p(x,b),\n   p(b,c),\n   c = (b+1),\n   x = (c+2).",
            toString(*getClauses(program, "p")[0]));
}

/**
 * Test that copies of relations are removed by RemoveRelationCopiesTransformer
 *
 * A(1, 2).
 * B(x, y) :- A(x, y).
 * C(x, y) :- B(x, y).
 * D(x, y) :- C(x, y).
 *
 * -> D(x, y) :- A(x, y).
 *
 *  B and C can be removed
 *
 */

TEST(AstTransformers, RemoveRelationCopies) {
    ErrorReport errorReport;
    DebugReport debugReport;
    // load some test program
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .type D = number
                .decl a(a:D,b:D)
                .decl b(a:D,b:D)
                .decl c(a:D,b:D)
                .decl d(a:D,b:D)

                a(1,2).
                b(x,y) :- a(x,y).
                c(x,y) :- b(x,y).

                d(x,y) :- b(x,y), c(y,x).

            )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    EXPECT_EQ(4, program.getRelations().size());

    RemoveRelationCopiesTransformer::removeRelationCopies(*tu);

    EXPECT_EQ(2, program.getRelations().size());
}

/**
 * Test that copies of relations are removed by RemoveRelationCopiesTransformer
 *
 * A(1, 2).
 * B(x, y) :- A(x, y).
 * C(x, y) :- B(x, y).
 * D(x, y) :- C(x, y).
 * .output C
 *
 * -> C(x, y) :- A(x, y).
 * -> D(x, y) :- C(x, y).
 *
 *  B can be removed, but not C as C is output
 *
 */
TEST(AstTransformers, RemoveRelationCopiesOutput) {
    ErrorReport errorReport;
    DebugReport debugReport;
    // load some test program
    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .type D = number
                .decl a(a:D,b:D)
                .decl b(a:D,b:D)
                .decl c(a:D,b:D)
                .output c
                .decl d(a:D,b:D)

                a(1,2).
                b(x,y) :- a(x,y).
                c(x,y) :- b(x,y).

                d(x,y) :- b(x,y), c(y,x).

            )",
            errorReport, debugReport);

    AstProgram& program = *tu->getProgram();

    EXPECT_EQ(4, program.getRelations().size());

    RemoveRelationCopiesTransformer::removeRelationCopies(*tu);

    EXPECT_EQ(3, program.getRelations().size());
}

/**
 * Test the equivalence (or lack of equivalence) of clauses using the MinimiseProgramTransfomer.
 */
TEST(AstTransformers, CheckClausalEquivalence) {
    ErrorReport errorReport;
    DebugReport debugReport;

    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .decl A(x:number, y:number)
                .decl B(x:number)
                .decl C(x:number)

                A(0,0).
                A(0,0).
                A(0,1).

                B(1).

                C(z) :- A(z,y), A(z,x), x != 3, x < y, !B(x), y > 3, B(y).
                C(r) :- A(r,y), A(r,x), x != 3, x < y, !B(y), y > 3, B(y), B(x), x < y.
                C(x) :- A(x,a), a != 3, !B(a), A(x,b), b > 3, B(c), a < b, c = b.
            )",
            errorReport, debugReport);

    const auto& program = *tu->getProgram();

    // Resolve aliases to remove trivial equalities
    std::make_unique<ResolveAliasesTransformer>()->apply(*tu);
    auto aClauses = getClauses(program, "A");
    auto bClauses = getClauses(program, "B");
    auto cClauses = getClauses(program, "C");

    EXPECT_EQ(3, aClauses.size());
    EXPECT_EQ("A(0,0).", toString(*aClauses[0]));
    EXPECT_EQ("A(0,0).", toString(*aClauses[1]));
    EXPECT_EQ("A(0,1).", toString(*aClauses[2]));

    EXPECT_EQ(1, bClauses.size());
    EXPECT_EQ("B(1).", toString(*bClauses[0]));

    EXPECT_EQ(3, cClauses.size());
    EXPECT_EQ("C(z) :- \n   A(z,y),\n   A(z,x),\n   x != 3,\n   x < y,\n   !B(x),\n   y > 3,\n   B(y).",
            toString(*cClauses[0]));
    EXPECT_EQ(
            "C(r) :- \n   A(r,y),\n   A(r,x),\n   x != 3,\n   x < y,\n   !B(y),\n   y > 3,\n   B(y),\n   "
            "B(x).",
            toString(*cClauses[1]));
    EXPECT_EQ("C(x) :- \n   A(x,a),\n   a != 3,\n   !B(a),\n   A(x,b),\n   b > 3,\n   B(b),\n   a < b.",
            toString(*cClauses[2]));

    // Check equivalence of these clauses
    // -- A
    const auto normA0 = NormalisedClause(aClauses[0]);
    const auto normA1 = NormalisedClause(aClauses[1]);
    const auto normA2 = NormalisedClause(aClauses[2]);
    EXPECT_TRUE(MinimiseProgramTransformer::areBijectivelyEquivalent(normA0, normA1));
    EXPECT_TRUE(MinimiseProgramTransformer::areBijectivelyEquivalent(normA1, normA0));
    EXPECT_FALSE(MinimiseProgramTransformer::areBijectivelyEquivalent(normA1, normA2));
    EXPECT_FALSE(MinimiseProgramTransformer::areBijectivelyEquivalent(normA0, normA2));

    // -- C
    const auto normC0 = NormalisedClause(cClauses[0]);
    const auto normC1 = NormalisedClause(cClauses[1]);
    const auto normC2 = NormalisedClause(cClauses[2]);
    EXPECT_TRUE(MinimiseProgramTransformer::areBijectivelyEquivalent(normC0, normC2));
    EXPECT_FALSE(MinimiseProgramTransformer::areBijectivelyEquivalent(normC0, normC1));
    EXPECT_FALSE(MinimiseProgramTransformer::areBijectivelyEquivalent(normC2, normC1));

    // Make sure equivalent (and only equivalent) clauses are removed by the minimiser
    std::make_unique<MinimiseProgramTransformer>()->apply(*tu);
    auto aMinClauses = getClauses(program, "A");
    auto bMinClauses = getClauses(program, "B");
    auto cMinClauses = getClauses(program, "C");

    EXPECT_EQ(2, aMinClauses.size());
    EXPECT_EQ("A(0,0).", toString(*aMinClauses[0]));
    EXPECT_EQ("A(0,1).", toString(*aMinClauses[1]));

    EXPECT_EQ(1, bMinClauses.size());
    EXPECT_EQ("B(1).", toString(*bMinClauses[0]));

    EXPECT_EQ(2, cMinClauses.size());
    EXPECT_EQ("C(z) :- \n   A(z,y),\n   A(z,x),\n   x != 3,\n   x < y,\n   !B(x),\n   y > 3,\n   B(y).",
            toString(*cMinClauses[0]));
    EXPECT_EQ(
            "C(r) :- \n   A(r,y),\n   A(r,x),\n   x != 3,\n   x < y,\n   !B(y),\n   y > 3,\n   B(y),\n   "
            "B(x).",
            toString(*cMinClauses[1]));
}

/**
 * Test the equivalence (or lack of equivalence) of aggregators using the MinimiseProgramTransfomer.
 */
TEST(AstTransformers, CheckAggregatorEquivalence) {
    ErrorReport errorReport;
    DebugReport debugReport;

    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .decl A,B,C,D(X:number) input
                // first and second are equivalent
                D(X) :-
                    B(X),
                    X < max Y : { C(Y), B(Y), Y < 2 },
                    A(Z),
                    Z = sum A : { C(A), B(A), A > count : { A(M), C(M) } }.

                D(V) :-
                    B(V),
                    A(W),
                    W = sum test1 : { C(test1), B(test1), test1 > count : { C(X), A(X) } },
                    V < max test2 : { C(test2), B(test2), test2 < 2 }.

                // third not equivalent
                D(V) :-
                    B(V),
                    A(W),
                    W = min test1 : { C(test1), B(test1), test1 > count : { C(X), A(X) } },
                    V < max test2 : { C(test2), B(test2), test2 < 2 }.

                .output D()
            )",
            errorReport, debugReport);

    const auto& program = *tu->getProgram();
    std::make_unique<MinimiseProgramTransformer>()->apply(*tu);

    // A, B, C, D should still be the relations
    EXPECT_EQ(4, program.getRelations().size());
    EXPECT_NE(nullptr, getRelation(program, "A"));
    EXPECT_NE(nullptr, getRelation(program, "B"));
    EXPECT_NE(nullptr, getRelation(program, "C"));
    EXPECT_NE(nullptr, getRelation(program, "D"));

    // D should now only have the two clauses non-equivalent clauses
    const auto& dClauses = getClauses(program, "D");
    EXPECT_EQ(2, dClauses.size());
    EXPECT_EQ(
            "D(X) :- \n   B(X),\n   X < max Y : { C(Y),B(Y),Y < 2 },\n   A(Z),\n   Z = sum A : { C(A),B(A),A "
            "> count : { A(M),C(M) } }.",
            toString(*dClauses[0]));
    EXPECT_EQ(
            "D(V) :- \n   B(V),\n   A(W),\n   W = min test1 : { C(test1),B(test1),test1 > count : { "
            "C(X),A(X) } },\n   V < max test2 : { C(test2),B(test2),test2 < 2 }.",
            toString(*dClauses[1]));
}

/**
 * Test the removal of redundancies within clauses using the MinimiseProgramTransformer.
 *
 * In particular, the removal of:
 *      - intraclausal literals equivalent to another literal in the body
 *          e.g. a(x) :- b(x), b(x), c(x). --> a(x) :- b(x), c(x).
 *      - clauses that are only trivially satisfiable
 *          e.g. a(x) :- a(x), x != 0. is only true if a(x) is already true
 */
TEST(AstTransformers, RemoveClauseRedundancies) {
    ErrorReport errorReport;
    DebugReport debugReport;

    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                .decl a,b,c(X:number)
                a(0).
                b(1).
                c(X) :- b(X).

                a(X) :- b(X), c(X).
                a(X) :- a(X).
                a(X) :- a(X), X != 1.

                q(X) :- a(X).

                .decl q(X:number)
                .output q()
            )",
            errorReport, debugReport);

    const auto& program = *tu->getProgram();

    // Invoking the `RemoveRelationCopiesTransformer` to create some extra redundancy
    // In particular: The relation `c` will be replaced with `b` throughout, creating
    // the clause b(x) :- b(x).
    std::make_unique<RemoveRelationCopiesTransformer>()->apply(*tu);
    EXPECT_EQ(nullptr, getRelation(program, "c"));
    auto bIntermediateClauses = getClauses(program, "b");
    EXPECT_EQ(2, bIntermediateClauses.size());
    EXPECT_EQ("b(1).", toString(*bIntermediateClauses[0]));
    EXPECT_EQ("b(X) :- \n   b(X).", toString(*bIntermediateClauses[1]));

    // Attempt to minimise the program
    std::make_unique<MinimiseProgramTransformer>()->apply(*tu);
    EXPECT_EQ(3, program.getRelations().size());

    auto aClauses = getClauses(program, "a");
    EXPECT_EQ(2, aClauses.size());
    EXPECT_EQ("a(0).", toString(*aClauses[0]));
    EXPECT_EQ("a(X) :- \n   b(X).", toString(*aClauses[1]));

    auto bClauses = getClauses(program, "b");
    EXPECT_EQ(1, bClauses.size());
    EXPECT_EQ("b(1).", toString(*bClauses[0]));

    auto qClauses = getClauses(program, "q");
    EXPECT_EQ(1, qClauses.size());
    EXPECT_EQ("q(X) :- \n   a(X).", toString(*qClauses[0]));
}

/**
 * Test the magic-set transformation on an example that covers all subtransformers, namely:
 *      (1) NormaliseDatabaseTransformer
 *      (2) LabelDatabaseTransformer
 *      (3) AdornDatabaseTransformer
 *      (4) MagicSetTransformer
 */
TEST(AstTransformers, MagicSetComprehensive) {
    ErrorReport e;
    DebugReport d;

    std::unique_ptr<AstTranslationUnit> tu = ParserDriver::parseTranslationUnit(
            R"(
                // Stratum 0 - Base Relations
                .decl BaseOne(X:number) magic
                .decl BaseTwo(X:number) magic
                .input BaseOne, BaseTwo

                // Stratum 1 [depends on: 0]
                .decl A(X:number) magic
                .decl B(X:number) magic
                A(X) :- BaseOne(X).
                A(X) :- BaseOne(X), B(X).
                B(X) :- BaseTwo(X), A(X).

                // Stratum 2 [depends on: 0,1]
                .decl C(X:number) magic
                C(X) :- BaseTwo(X), A(X), B(X), X != 1.

                // Stratum 3 [depends on: 0,1]
                .decl R(X:number) magic
                R(X) :- BaseTwo(X), A(X), B(X), X != 0.

                // Stratum 4 [depends on: 0,1,2,3]
                .decl D(X:number) magic
                D(X) :- BaseOne(X), A(X), !C(X), !R(X).

                // Stratum 4 - Query [depends on: 0,1,4]
                .decl Query(X:number) magic
                .output Query
                Query(X) :- BaseOne(X), D(X), A(X).
            )",
            e, d);

    auto& program = *tu->getProgram();

    // Test helpers
    auto mappifyRelations = [&](const AstProgram& program) {
        std::map<std::string, std::multiset<std::string>> result;
        for (const auto* rel : program.getRelations()) {
            std::multiset<std::string> clauseStrings;
            auto relName = rel->getQualifiedName();
            for (const auto* clause : getClauses(program, rel->getQualifiedName())) {
                clauseStrings.insert(toString(*clause));
            }
            result[toString(relName)] = clauseStrings;
        }
        return result;
    };
    auto checkRelMapEq = [&](const std::map<std::string, std::multiset<std::string>> left,
                                 const std::map<std::string, std::multiset<std::string>> right) {
        EXPECT_EQ(left.size(), right.size());
        for (const auto& [name, clauses] : left) {
            EXPECT_TRUE(contains(right, name));
            EXPECT_EQ(clauses, right.at(name));
        }
    };

    /* Stage 1: Database normalisation */
    // Constants should now be extracted from the inequality constraints
    std::make_unique<MagicSetTransformer::NormaliseDatabaseTransformer>()->apply(*tu);
    const auto relations1 = program.getRelations();
    EXPECT_EQ(8, program.getRelations().size());
    EXPECT_EQ(7, program.getClauses().size());

    auto expectedNormalisation = std::map<std::string, std::multiset<std::string>>({
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"A", {"A(X) :- \n   BaseOne(X).", "A(X) :- \n   BaseOne(X),\n   B(X)."}},
            {"B", {"B(X) :- \n   BaseTwo(X),\n   A(X)."}},
            {"C", {"C(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"R", {"R(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 0."}},
            {"D", {"D(X) :- \n   BaseOne(X),\n   A(X),\n   !C(X),\n   !R(X)."}},
            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D(X),\n   A(X)."}},
    });
    checkRelMapEq(expectedNormalisation, mappifyRelations(program));

    /* Stage 2: Database negative and positive labelling, keeping only the relevant ones */
    /* Stage 2.1: Negative labelling */
    std::make_unique<MagicSetTransformer::LabelDatabaseTransformer::NegativeLabellingTransformer>()->apply(
            *tu);
    EXPECT_EQ(14, program.getRelations().size());
    EXPECT_EQ(14, program.getClauses().size());

    auto expectedNegLabelling = std::map<std::string, std::multiset<std::string>>({
            // Original strata - neglabels should appear on all negated appearances of relations
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"A", {"A(X) :- \n   BaseOne(X).", "A(X) :- \n   BaseOne(X),\n   B(X)."}},
            {"B", {"B(X) :- \n   BaseTwo(X),\n   A(X)."}},
            {"C", {"C(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"R", {"R(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 0."}},
            {"D", {"D(X) :- \n   BaseOne(X),\n   A(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},
            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D(X),\n   A(X)."}},

            // Neglaelled strata - relations should be copied and neglabelled one stratum at a time
            {"@neglabel.A", {"@neglabel.A(X) :- \n   BaseOne(X).",
                                    "@neglabel.A(X) :- \n   BaseOne(X),\n   @neglabel.B(X)."}},
            {"@neglabel.B", {"@neglabel.B(X) :- \n   BaseTwo(X),\n   @neglabel.A(X)."}},
            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   "
                             "@abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   "
                             "@abdul0 = 0."}},
            {"@neglabel.D", {"@neglabel.D(X) :- \n   BaseOne(X),\n   A(X),\n   !@neglabel.C(X),\n   "
                             "!@neglabel.R(X)."}},
            {"@neglabel.Query", {"@neglabel.Query(X) :- \n   BaseOne(X),\n   D(X),\n   A(X)."}},
    });
    checkRelMapEq(expectedNegLabelling, mappifyRelations(program));

    /* Stage 2.2: Positive labelling */
    std::make_unique<MagicSetTransformer::LabelDatabaseTransformer::PositiveLabellingTransformer>()->apply(
            *tu);
    EXPECT_EQ(33, program.getRelations().size());
    EXPECT_EQ(27, program.getClauses().size());

    auto expectedPosLabelling = std::map<std::string, std::multiset<std::string>>({
            // Original strata should remain the same
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"A", {"A(X) :- \n   BaseOne(X).", "A(X) :- \n   BaseOne(X),\n   B(X)."}},
            {"B", {"B(X) :- \n   BaseTwo(X),\n   A(X)."}},
            {"C", {"C(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"R", {"R(X) :- \n   BaseTwo(X),\n   A(X),\n   B(X),\n   X != @abdul0,\n   @abdul0 = 0."}},
            {"D", {"D(X) :- \n   BaseOne(X),\n   A(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},
            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D(X),\n   A(X)."}},

            // Poslabelled + neglabelled strata - poslabel all positive derived literals
            {"@neglabel.A", {"@neglabel.A(X) :- \n   BaseOne(X).",
                                    "@neglabel.A(X) :- \n   BaseOne(X),\n   @neglabel.B(X)."}},
            {"@neglabel.B", {"@neglabel.B(X) :- \n   BaseTwo(X),\n   @neglabel.A(X)."}},
            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   @poscopy_1.A(X),\n   @poscopy_1.B(X),\n "
                             "  X != @abdul0,\n   "
                             "@abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   @poscopy_2.A(X),\n   @poscopy_2.B(X),\n "
                             "  X != @abdul0,\n   "
                             "@abdul0 = 0."}},
            {"@neglabel.D",
                    {"@neglabel.D(X) :- \n   BaseOne(X),\n   @poscopy_3.A(X),\n   !@neglabel.C(X),\n   "
                     "!@neglabel.R(X)."}},
            {"@neglabel.Query",
                    {"@neglabel.Query(X) :- \n   BaseOne(X),\n   @poscopy_1.D(X),\n   @poscopy_4.A(X)."}},

            // Copies of input stratum
            {"@poscopy_1.BaseOne", {}},
            {"@poscopy_1.BaseTwo", {}},
            {"@poscopy_2.BaseOne", {}},
            {"@poscopy_2.BaseTwo", {}},
            {"@poscopy_3.BaseOne", {}},
            {"@poscopy_3.BaseTwo", {}},
            {"@poscopy_4.BaseOne", {}},
            {"@poscopy_4.BaseTwo", {}},
            {"@poscopy_5.BaseOne", {}},
            {"@poscopy_5.BaseTwo", {}},

            // Copies of {A,B} stratum
            {"@poscopy_1.A", {"@poscopy_1.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_1.A(X) :- \n   BaseOne(X),\n   @poscopy_1.B(X)."}},
            {"@poscopy_1.B", {"@poscopy_1.B(X) :- \n   BaseTwo(X),\n   @poscopy_1.A(X)."}},
            {"@poscopy_2.A", {"@poscopy_2.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_2.A(X) :- \n   BaseOne(X),\n   @poscopy_2.B(X)."}},
            {"@poscopy_2.B", {"@poscopy_2.B(X) :- \n   BaseTwo(X),\n   @poscopy_2.A(X)."}},
            {"@poscopy_3.A", {"@poscopy_3.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_3.A(X) :- \n   BaseOne(X),\n   @poscopy_3.B(X)."}},
            {"@poscopy_3.B", {"@poscopy_3.B(X) :- \n   BaseTwo(X),\n   @poscopy_3.A(X)."}},
            {"@poscopy_4.A", {"@poscopy_4.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_4.A(X) :- \n   BaseOne(X),\n   @poscopy_4.B(X)."}},
            {"@poscopy_4.B", {"@poscopy_4.B(X) :- \n   BaseTwo(X),\n   @poscopy_4.A(X)."}},

            // Copies of {D} stratum
            {"@poscopy_1.D", {"@poscopy_1.D(X) :- \n   BaseOne(X),\n   @poscopy_4.A(X),\n   "
                              "!@neglabel.C(X),\n   !@neglabel.R(X)."}},
    });
    checkRelMapEq(expectedPosLabelling, mappifyRelations(program));

    /* Stage 2.3: Remove unnecessary labelled relations */
    std::make_unique<RemoveRedundantRelationsTransformer>()->apply(*tu);
    EXPECT_EQ(12, program.getRelations().size());
    EXPECT_EQ(13, program.getClauses().size());

    auto expectedFullLabelling = std::map<std::string, std::multiset<std::string>>({
            // Original strata
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"A", {"A(X) :- \n   BaseOne(X).", "A(X) :- \n   BaseOne(X),\n   B(X)."}},
            {"B", {"B(X) :- \n   BaseTwo(X),\n   A(X)."}},
            {"D", {"D(X) :- \n   BaseOne(X),\n   A(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},
            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D(X),\n   A(X)."}},

            // Neglabelled strata
            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   @poscopy_1.A(X),\n   @poscopy_1.B(X),\n "
                             "  X != @abdul0,\n   "
                             "@abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   @poscopy_2.A(X),\n   @poscopy_2.B(X),\n "
                             "  X != @abdul0,\n   "
                             "@abdul0 = 0."}},

            // Poslabelled strata
            {"@poscopy_1.A", {"@poscopy_1.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_1.A(X) :- \n   BaseOne(X),\n   @poscopy_1.B(X)."}},
            {"@poscopy_1.B", {"@poscopy_1.B(X) :- \n   BaseTwo(X),\n   @poscopy_1.A(X)."}},
            {"@poscopy_2.A", {"@poscopy_2.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_2.A(X) :- \n   BaseOne(X),\n   @poscopy_2.B(X)."}},
            {"@poscopy_2.B", {"@poscopy_2.B(X) :- \n   BaseTwo(X),\n   @poscopy_2.A(X)."}},
    });
    checkRelMapEq(expectedFullLabelling, mappifyRelations(program));

    /* Stage 3: Database adornment */
    /* Stage 3.1: Adornment algorithm */
    std::make_unique<MagicSetTransformer::AdornDatabaseTransformer>()->apply(*tu);
    EXPECT_EQ(19, program.getRelations().size());
    EXPECT_EQ(23, program.getClauses().size());

    auto expectedAdornment = std::map<std::string, std::multiset<std::string>>({
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"A", {"A(X) :- \n   BaseOne(X).", "A(X) :- \n   BaseOne(X),\n   B(X)."}},
            {"B", {"B(X) :- \n   BaseTwo(X),\n   A(X)."}},
            {"D", {"D(X) :- \n   BaseOne(X),\n   A(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},

            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X),\n   "
                             "@poscopy_1.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X),\n   "
                             "@poscopy_2.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 0."}},
            {"@poscopy_1.A", {"@poscopy_1.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_1.A(X) :- \n   BaseOne(X),\n   @poscopy_1.B(X)."}},
            {"@poscopy_1.B", {"@poscopy_1.B(X) :- \n   BaseTwo(X),\n   @poscopy_1.A(X)."}},
            {"@poscopy_2.A", {"@poscopy_2.A(X) :- \n   BaseOne(X).",
                                     "@poscopy_2.A(X) :- \n   BaseOne(X),\n   @poscopy_2.B(X)."}},
            {"@poscopy_2.B", {"@poscopy_2.B(X) :- \n   BaseTwo(X),\n   @poscopy_2.A(X)."}},

            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D.{b}(X),\n   A.{b}(X)."}},
            {"A.{b}", {"A.{b}(X) :- \n   BaseOne(X).", "A.{b}(X) :- \n   BaseOne(X),\n   B.{b}(X)."}},
            {"B.{b}", {"B.{b}(X) :- \n   BaseTwo(X),\n   A.{b}(X)."}},
            {"D.{b}",
                    {"D.{b}(X) :- \n   BaseOne(X),\n   A.{b}(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},
            {"@poscopy_1.A.{b}", {"@poscopy_1.A.{b}(X) :- \n   BaseOne(X).",
                                         "@poscopy_1.A.{b}(X) :- \n   BaseOne(X),\n   @poscopy_1.B.{b}(X)."}},
            {"@poscopy_1.B.{b}", {"@poscopy_1.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X)."}},
            {"@poscopy_2.A.{b}", {"@poscopy_2.A.{b}(X) :- \n   BaseOne(X).",
                                         "@poscopy_2.A.{b}(X) :- \n   BaseOne(X),\n   @poscopy_2.B.{b}(X)."}},
            {"@poscopy_2.B.{b}", {"@poscopy_2.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X)."}},

    });
    checkRelMapEq(expectedAdornment, mappifyRelations(program));

    /* Stage 2.3: Remove no longer necessary relations */
    std::make_unique<RemoveRedundantRelationsTransformer>()->apply(*tu);
    EXPECT_EQ(12, program.getRelations().size());
    EXPECT_EQ(13, program.getClauses().size());

    auto expectedFinalAdornment = std::map<std::string, std::multiset<std::string>>({
            {"BaseOne", {}},
            {"BaseTwo", {}},
            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X),\n   "
                             "@poscopy_1.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X),\n   "
                             "@poscopy_2.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 0."}},
            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D.{b}(X),\n   A.{b}(X)."}},
            {"A.{b}", {"A.{b}(X) :- \n   BaseOne(X).", "A.{b}(X) :- \n   BaseOne(X),\n   B.{b}(X)."}},
            {"B.{b}", {"B.{b}(X) :- \n   BaseTwo(X),\n   A.{b}(X)."}},
            {"D.{b}",
                    {"D.{b}(X) :- \n   BaseOne(X),\n   A.{b}(X),\n   !@neglabel.C(X),\n   !@neglabel.R(X)."}},
            {"@poscopy_1.A.{b}", {"@poscopy_1.A.{b}(X) :- \n   BaseOne(X).",
                                         "@poscopy_1.A.{b}(X) :- \n   BaseOne(X),\n   @poscopy_1.B.{b}(X)."}},
            {"@poscopy_1.B.{b}", {"@poscopy_1.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X)."}},
            {"@poscopy_2.A.{b}", {"@poscopy_2.A.{b}(X) :- \n   BaseOne(X).",
                                         "@poscopy_2.A.{b}(X) :- \n   BaseOne(X),\n   @poscopy_2.B.{b}(X)."}},
            {"@poscopy_2.B.{b}", {"@poscopy_2.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X)."}},

    });
    checkRelMapEq(expectedFinalAdornment, mappifyRelations(program));

    /* Stage 4: MST core transformation */
    std::make_unique<MagicSetTransformer::MagicSetCoreTransformer>()->apply(*tu);
    EXPECT_EQ(19, program.getRelations().size());
    EXPECT_EQ(26, program.getClauses().size());

    auto finalProgram = std::map<std::string, std::multiset<std::string>>({
            {"BaseOne", {}},
            {"BaseTwo", {}},

            {"@neglabel.C", {"@neglabel.C(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X),\n   "
                             "@poscopy_1.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 1."}},
            {"@neglabel.R", {"@neglabel.R(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X),\n   "
                             "@poscopy_2.B.{b}(X),\n   X != @abdul0,\n   @abdul0 = 0."}},

            {"A.{b}", {"A.{b}(X) :- \n   @magic.A.{b}(X),\n   BaseOne(X),\n   B.{b}(X).",
                              "A.{b}(X) :- \n   @magic.A.{b}(X),\n   BaseOne(X)."}},
            {"B.{b}", {"B.{b}(X) :- \n   @magic.B.{b}(X),\n   BaseTwo(X),\n   A.{b}(X)."}},
            {"D.{b}", {"D.{b}(X) :- \n   @magic.D.{b}(X),\n   BaseOne(X),\n   A.{b}(X),\n   "
                       "!@neglabel.C(X),\n   !@neglabel.R(X)."}},

            {"@poscopy_1.A.{b}", {"@poscopy_1.A.{b}(X) :- \n   @magic.@poscopy_1.A.{b}(X),\n   BaseOne(X).",
                                         "@poscopy_1.A.{b}(X) :- \n   @magic.@poscopy_1.A.{b}(X),\n   "
                                         "BaseOne(X),\n   @poscopy_1.B.{b}(X)."}},
            {"@poscopy_1.B.{b}", {"@poscopy_1.B.{b}(X) :- \n   @magic.@poscopy_1.B.{b}(X),\n   BaseTwo(X),\n "
                                  "  @poscopy_1.A.{b}(X)."}},
            {"@poscopy_2.A.{b}", {"@poscopy_2.A.{b}(X) :- \n   @magic.@poscopy_2.A.{b}(X),\n   BaseOne(X).",
                                         "@poscopy_2.A.{b}(X) :- \n   @magic.@poscopy_2.A.{b}(X),\n   "
                                         "BaseOne(X),\n   @poscopy_2.B.{b}(X)."}},
            {"@poscopy_2.B.{b}", {"@poscopy_2.B.{b}(X) :- \n   @magic.@poscopy_2.B.{b}(X),\n   BaseTwo(X),\n "
                                  "  @poscopy_2.A.{b}(X)."}},

            {"Query", {"Query(X) :- \n   BaseOne(X),\n   D.{b}(X),\n   A.{b}(X)."}},

            // Magic rules
            {"@magic.A.{b}", {"@magic.A.{b}(X) :- \n   @magic.B.{b}(X),\n   BaseTwo(X).",
                                     "@magic.A.{b}(X) :- \n   BaseOne(X),\n   D.{b}(X).",
                                     "@magic.A.{b}(X) :- \n   @magic.D.{b}(X),\n   BaseOne(X)."}},
            {"@magic.B.{b}", {"@magic.B.{b}(X) :- \n   @magic.A.{b}(X),\n   BaseOne(X)."}},
            {"@magic.D.{b}", {"@magic.D.{b}(X) :- \n   BaseOne(X)."}},
            {"@magic.@poscopy_1.A.{b}", {"@magic.@poscopy_1.A.{b}(X) :- \n   BaseTwo(X),\n   @abdul0 = 1.",
                                                "@magic.@poscopy_1.A.{b}(X) :- \n   "
                                                "@magic.@poscopy_1.B.{b}(X),\n   BaseTwo(X)."}},
            {"@magic.@poscopy_2.A.{b}", {"@magic.@poscopy_2.A.{b}(X) :- \n   BaseTwo(X),\n   @abdul0 = 0.",
                                                "@magic.@poscopy_2.A.{b}(X) :- \n   "
                                                "@magic.@poscopy_2.B.{b}(X),\n   BaseTwo(X)."}},
            {"@magic.@poscopy_1.B.{b}",
                    {"@magic.@poscopy_1.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_1.A.{b}(X),\n   @abdul0 = "
                     "1.",
                            "@magic.@poscopy_1.B.{b}(X) :- \n   @magic.@poscopy_1.A.{b}(X),\n   "
                            "BaseOne(X)."}},
            {"@magic.@poscopy_2.B.{b}",
                    {"@magic.@poscopy_2.B.{b}(X) :- \n   BaseTwo(X),\n   @poscopy_2.A.{b}(X),\n   @abdul0 = "
                     "0.",
                            "@magic.@poscopy_2.B.{b}(X) :- \n   @magic.@poscopy_2.A.{b}(X),\n   "
                            "BaseOne(X)."}},
    });
    checkRelMapEq(finalProgram, mappifyRelations(program));
}  // namespace test

}  // namespace test
}  // namespace souffle
