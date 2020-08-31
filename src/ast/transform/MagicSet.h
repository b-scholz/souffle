/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file MagicSet.h
 *
 * Define classes and functionality related to the magic set transformation.
 *
 ***********************************************************************/

#pragma once

#include "ast/Argument.h"
#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/Literal.h"
#include "ast/QualifiedName.h"
#include "ast/Variable.h"
#include "ast/analysis/Analysis.h"
#include "ast/transform/Pipeline.h"
#include "ast/transform/RemoveRedundantRelations.h"
#include "ast/transform/Transformer.h"
#include "ast/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

class AstTranslationUnit;

/**
 * Magic Set Transformation.
 * Involves four stages:
 *      (1) NormaliseDatabaseTransformer, for assumptions to hold
 *      (2) LabelDatabaseTransformer, to support negation
 *      (3) AdornDatabaseTransformer, to annotate information flow
 *      (4) MagicSetCoreTransformer, to perform the core magifying transformation
 */
class MagicSetTransformer : public PipelineTransformer {
public:
    class NormaliseDatabaseTransformer;
    class LabelDatabaseTransformer;
    class AdornDatabaseTransformer;
    class MagicSetCoreTransformer;

    MagicSetTransformer()
            : PipelineTransformer(std::make_unique<NormaliseDatabaseTransformer>(),
                      std::make_unique<LabelDatabaseTransformer>(),
                      std::make_unique<RemoveRedundantRelationsTransformer>(),
                      std::make_unique<AdornDatabaseTransformer>(),
                      std::make_unique<RemoveRedundantRelationsTransformer>(),
                      std::make_unique<MagicSetCoreTransformer>()) {}

    std::string getName() const override {
        return "MagicSetTransformer";
    }

    MagicSetTransformer* clone() const override {
        return new MagicSetTransformer();
    }

private:
    bool transform(AstTranslationUnit& tu) override {
        return shouldRun(tu) ? PipelineTransformer::transform(tu) : false;
    }

    /** Determines whether any part of the MST should be run. */
    static bool shouldRun(const AstTranslationUnit& tu);

    /**
     * Gets set of relations to ignore during the MST process.
     * Ignored relations are relations that should not be copied or altered beyond normalisation.
     */
    static std::set<AstQualifiedName> getIgnoredRelations(const AstTranslationUnit& tu);
};

/**
 * Database normaliser for MST.
 * Effects:
 *  - Partitions database into [input|intermediate|queries]
 *  - Normalises all arguments and constraints
 * Prerequisite for adornment.
 */
class MagicSetTransformer::NormaliseDatabaseTransformer : public AstTransformer {
public:
    std::string getName() const override {
        return "NormaliseDatabaseTransformer";
    }

    NormaliseDatabaseTransformer* clone() const override {
        return new NormaliseDatabaseTransformer();
    }

private:
    bool transform(AstTranslationUnit& translationUnit) override;

    /**
     * Partitions the input and output relations.
     * Program will no longer have relations that are both input and output.
     */
    static bool partitionIO(AstTranslationUnit& translationUnit);

    /**
     * Separates the IDB from the EDB, so that they are disjoint.
     * Program will no longer have input relations that appear as the head of clauses.
     */
    static bool extractIDB(AstTranslationUnit& translationUnit);

    /**
     * Extracts output relations into separate simple query relations,
     * so that they are unused in any other rules.
     * Programs will only contain output relations which:
     *      (1) have exactly one rule defining them
     *      (2) do not appear in other rules
     */
    static bool querifyOutputRelations(AstTranslationUnit& translationUnit);

    /**
     * Normalise all arguments within each clause.
     * All arguments in all clauses will now be either:
     *      (1) a variable, or
     *      (2) the RHS of a `<var> = <arg>` constraint
     */
    static bool normaliseArguments(AstTranslationUnit& translationUnit);
};

/**
 * Database labeller. Runs the magic-set labelling algorithm.
 * Necessary for supporting negation in MST.
 */
class MagicSetTransformer::LabelDatabaseTransformer : public PipelineTransformer {
public:
    class NegativeLabellingTransformer;
    class PositiveLabellingTransformer;

    LabelDatabaseTransformer()
            : PipelineTransformer(std::make_unique<NegativeLabellingTransformer>(),
                      std::make_unique<PositiveLabellingTransformer>()) {}

    std::string getName() const override {
        return "LabelDatabaseTransformer";
    }

    LabelDatabaseTransformer* clone() const override {
        return new LabelDatabaseTransformer();
    }

private:
    /** Check if a relation is negatively labelled. */
    static bool isNegativelyLabelled(const AstQualifiedName& name);
};

/**
 * Runs the first stage of the labelling algorithm.
 * Separates out negated appearances of relations from the main SCC graph, preventing them from affecting
 * stratification once magic dependencies are added.
 */
class MagicSetTransformer::LabelDatabaseTransformer::NegativeLabellingTransformer : public AstTransformer {
public:
    std::string getName() const override {
        return "NegativeLabellingTransformer";
    }

    NegativeLabellingTransformer* clone() const override {
        return new NegativeLabellingTransformer();
    }

private:
    bool transform(AstTranslationUnit& translationUnit) override;

    /** Provide a unique name for negatively-labelled relations. */
    static AstQualifiedName getNegativeLabel(const AstQualifiedName& name);
};

/**
 * Runs the second stage of the labelling algorithm.
 * Separates out the dependencies of negatively labelled atoms from the main SCC graph, preventing them from
 * affecting stratification after magic.
 * Note: Negative labelling must have been run first.
 */
class MagicSetTransformer::LabelDatabaseTransformer::PositiveLabellingTransformer : public AstTransformer {
public:
    std::string getName() const override {
        return "PositiveLabellingTransformer";
    }

    PositiveLabellingTransformer* clone() const override {
        return new PositiveLabellingTransformer();
    }

private:
    bool transform(AstTranslationUnit& translationUnit) override;

    /** Provide a unique name for a positively labelled relation copy. */
    static AstQualifiedName getPositiveLabel(const AstQualifiedName& name, size_t count);
};

/**
 * Database adornment.
 * Adorns the rules of a database with variable flow and binding information.
 * Prerequisite for the magic set transformation.
 */
class MagicSetTransformer::AdornDatabaseTransformer : public AstTransformer {
public:
    std::string getName() const override {
        return "AdornDatabaseTransformer";
    }

    AdornDatabaseTransformer* clone() const override {
        return new AdornDatabaseTransformer();
    }

private:
    using adorned_predicate = std::pair<AstQualifiedName, std::string>;

    std::set<adorned_predicate> headAdornmentsToDo;
    std::set<AstQualifiedName> headAdornmentsSeen;

    std::vector<std::unique_ptr<AstClause>> adornedClauses;
    std::vector<std::unique_ptr<AstClause>> redundantClauses;
    std::set<AstQualifiedName> relationsToIgnore;

    bool transform(AstTranslationUnit& translationUnit) override;

    /** Get the unique identifier corresponding to an adorned predicate. */
    static AstQualifiedName getAdornmentID(
            const AstQualifiedName& relName, const std::string& adornmentMarker);

    /** Add an adornment to the ToDo queue if it hasn't been processed before. */
    void queueAdornment(const AstQualifiedName& relName, const std::string& adornmentMarker) {
        auto adornmentID = getAdornmentID(relName, adornmentMarker);
        if (!contains(headAdornmentsSeen, adornmentID)) {
            headAdornmentsToDo.insert(std::make_pair(relName, adornmentMarker));
            headAdornmentsSeen.insert(adornmentID);
        }
    }

    /** Check if any more relations need to be adorned. */
    bool hasAdornmentToProcess() const {
        return !headAdornmentsToDo.empty();
    }

    /** Pop off the next predicate adornment to process. **/
    adorned_predicate nextAdornmentToProcess() {
        assert(hasAdornmentToProcess() && "no adornment to pop");
        auto headAdornment = *(headAdornmentsToDo.begin());
        headAdornmentsToDo.erase(headAdornmentsToDo.begin());
        return headAdornment;
    }

    /** Returns the adorned version of a clause. */
    std::unique_ptr<AstClause> adornClause(const AstClause* clause, const std::string& adornmentMarker);
};

/**
 * Core section of the magic set transformer.
 * Creates all magic rules and relations based on the preceding adornment, and adds them into rules as needed.
 * Assumes that Normalisation, Labelling, and Adornment have all been performed.
 */
class MagicSetTransformer::MagicSetCoreTransformer : public AstTransformer {
public:
    std::string getName() const override {
        return "MagicSetCoreTransformer";
    }

    MagicSetCoreTransformer* clone() const override {
        return new MagicSetCoreTransformer();
    }

private:
    bool transform(AstTranslationUnit& translationUnit) override;

    /** Gets a unique magic identifier for a given adorned relation name */
    static AstQualifiedName getMagicName(const AstQualifiedName& name);

    /** Checks if a given relation name is adorned */
    static bool isAdorned(const AstQualifiedName& name);

    /** Retrieves an adornment encoded in a given relation name */
    static std::string getAdornment(const AstQualifiedName& name);

    /** Get all potentially-binding equality constraints in a clause */
    static std::vector<const AstBinaryConstraint*> getBindingEqualityConstraints(const AstClause* clause);

    /** Get the closure of the given set of variables under appearance in the given eq constraints */
    static void addRelevantVariables(
            std::set<std::string>& variables, const std::vector<const AstBinaryConstraint*> eqConstraints);

    /** Creates the magic atom associatd with the given (rel, adornment) pair */
    static std::unique_ptr<AstAtom> createMagicAtom(const AstAtom* atom);

    /** Creates the magic clause centred around the given magic atom */
    static std::unique_ptr<AstClause> createMagicClause(const AstAtom* atom,
            const std::vector<std::unique_ptr<AstAtom>>& constrainingAtoms,
            const std::vector<const AstBinaryConstraint*> eqConstraints);
};

}  // namespace souffle
