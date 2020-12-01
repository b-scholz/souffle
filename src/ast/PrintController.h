/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file PrintController.h
 *
 * Defines the print controller for printing clauses 
 *
 ***********************************************************************/

#pragma once 

#include <iostream>
#include <fstream>
#include <set>
#include <string>

class PrintController {
    // set of relations 
    std::set<std::string> relations;

    // mode 
    std::string mode; 

    PrintController() {
        std::ifstream myfile("relations.txt");
        if (myfile.is_open()) {
            bool first = true; 
            std::string line;
            while (getline(myfile, line)) {
                // first line is mode 
                if (first) { 
                    first = false; 
                    mode = line; 
                } else { 
                    // second or other lines are relation names 
                    relations.insert(line);
                } 
            }
            myfile.close();
        }
    }

public:
    static PrintController& getInstance() {
        static PrintController instance;
        return instance;
    };

    const std::set<std::string> &getRelations() const { 
        return relations; 
    } 

    const bool isRelation(const std::string name) const { 
        return relations.find(name) != relations.end(); 
    } 

    const std::string &getMode() const {
        return mode; 
    } 
};

#if 0
int main() {
    PrintController& pc = PrintController::getInstance();

    const auto &rels = pc.getRelations(); 
    const auto &mode = pc.getMode(); 
    std::cout << "Mode:" << pc.getMode() << std::endl; 
    for(const auto &r: rels) { 
        std::cout << r << std::endl; 
    }

    return 0;
}
#endif 
