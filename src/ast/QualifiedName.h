/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file QualifiedName.h
 *
 * Defines the qualified name class
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace souffle::ast {

/**
 * @class QualifiedName
 * @brief Qualified Name class defines fully/partially qualified names
 * to identify objects in components.
 */
class QualifiedName {
public:
    QualifiedName() : qualifiers() {}
    QualifiedName(const std::string& name) : qualifiers({name}) {}
    QualifiedName(const char* name) : QualifiedName(std::string(name)) {}
    QualifiedName(const std::vector<std::string> qualifiers) : qualifiers(qualifiers) {}
    QualifiedName(const QualifiedName&) = default;
    QualifiedName(QualifiedName&&) = default;
    QualifiedName& operator=(const QualifiedName&) = default;
    QualifiedName& operator=(QualifiedName&&) = default;

    /** append qualifiers */
    void append(std::string name) {
        qualifiers.push_back(std::move(name));
    }

    /** prepend qualifiers */
    void prepend(std::string name) {
        qualifiers.insert(qualifiers.begin(), std::move(name));
    }

    /** check for emptiness */
    bool empty() const {
        return qualifiers.empty();
    }

    /** get qualifiers */
    const std::vector<std::string>& getQualifiers() const {
        return qualifiers;
    }

    /** convert to a string separated by fullstop */
    std::string toString() const {
        std::stringstream ss;
        ss << join(qualifiers, "_");
        return ss.str();
    }

    bool operator==(const QualifiedName& other) const {
        return qualifiers == other.qualifiers;
    }

    bool operator!=(const QualifiedName& other) const {
        return !(*this == other);
    }

    bool operator<(const QualifiedName& other) const {
        return std::lexicographical_compare(
                qualifiers.begin(), qualifiers.end(), other.qualifiers.begin(), other.qualifiers.end());
    }

    /** print qualified name */
    void print(std::ostream& out) const {
        out << join(qualifiers, "_");
    }

    friend std::ostream& operator<<(std::ostream& out, const QualifiedName& id) {
        id.print(out);
        return out;
    }

private:
    /* array of name qualifiers */
    std::vector<std::string> qualifiers;
};

inline QualifiedName operator+(const std::string& name, const QualifiedName& id) {
    QualifiedName res = id;
    res.prepend(name);
    return res;
}

}  // namespace souffle::ast
