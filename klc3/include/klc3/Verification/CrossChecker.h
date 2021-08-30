//
// Created by liuzikai on 8/18/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_CROSSCHECKER_H
#define KLC3_CROSSCHECKER_H

#include "klc3/Core/State.h"

namespace klc3 {

class CrossChecker : protected WithBuilder {
public:

    CrossChecker(ExprBuilder *builder, Solver *solver) : WithBuilder(builder), solver(solver) {}

    /**
     * Compare a test state and the corresponding gold state.
     *
     * Theoretically, the cross comparison should fork the test state is some of its variable may & may not equal to
     * those of the gold state. But we mostly care about the condition that the test state is not equal to the gold.
     * So we apply the "not equal" constraints directly on the finalConstraints while discarding the equal situation.
     *
     * Comparison on output and registers will be performed in order. The newly added constraints will immediately
     * take effect on latter comparison. As many constraints are added to make the symbols different.
     *
     * @param goldState <- may set triggerNewIssue if callback is needed
     * @param testState <- may raise issue on it (and possibly set triggerNewIssue by IssuePackage)
     * @param finalConstraints <- UnEq constraint may get added
     */
    vector <Issue::Type> compare(State *goldState, State *testState, ConstraintSet &finalConstraints);

    struct ThingToCompare {
        enum Type {
            OUTPUT,
            REG,
            LAST_INST,
            MEM
        } type;

        uint16_t from;  // register start index for REG, memory start addr for MEM, not used for other types
        uint16_t to;  // register end index for REG, memory end addr for MEM, not used for other types

        bool dumpGold;
    };

    typedef vector<ThingToCompare> CheckList;

    void registerThingToCompare(Issue::Type issue, ThingToCompare thingToCompare);

    void deleteDefaultOutputCompare() { checkLists.erase(Issue::ERR_INCORRECT_OUTPUT); }

private:

    bool outputDiverge(State *goldState, State *testState, ConstraintSet &finalConstraints);

    bool compareRegisters(State *goldState, State *testState, ConstraintSet &finalConstraints, uint16_t from,
                          uint16_t to, llvm::raw_ostream &note);

    bool compareLastInst(State *goldState, State *testState, ConstraintSet &finalConstraints,
                         llvm::raw_ostream &note, bool dumpGold);

    bool compareMem(State *goldState, State *testState, ConstraintSet &finalConstraints,
                    uint16_t from, uint16_t to);


    Solver *solver;

    /**
     * Evaluate whether two expressions must be equal. If may but not must, the UnEq constraint is added to
     * finalConstraints.
     *
     * @param a
     * @param b
     * @param finalConstraints
     * @return True if two expressions must equal
     */
    bool exprMustEq(const ref <Expr> &a, const ref <Expr> &b, ConstraintSet &finalConstraints) const;

    static void generateIssueDesc(const Issue &issue, State *testState, const Assignment &assignment,
                                        void *ptrCallBackArg, string &desc);

    static void appendOutputToIssueDesc(State *testState, const Assignment &assignment, const State *goldState,
                                        string &desc);

    static void appendRegistersToIssueDesc(State *testState, const Assignment &assignment, const vector <Reg> &regs,
                                           const State *goldState, string &desc);

    static void appendMemToIssueDesc(State *testState, const Assignment &assignment, uint16_t start, uint16_t end,
                                     const State *goldState, string &desc);

    map<Issue::Type, CheckList> checkLists = {
            {Issue::ERR_INCORRECT_OUTPUT, {{ThingToCompare::OUTPUT, 0, 0, true}}}
    };

    struct CallBackArg {
        CheckList failedThings;    // make a copy as object address may not be persistent in STL containter
        const State *goldState;    // nullptr for not dumping gold
    };
};

}

#endif //KLC3_CROSSCHECKER_H
