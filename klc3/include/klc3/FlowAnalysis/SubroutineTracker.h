//
// Created by liuzikai on 10/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_SUBROUTINETRACKER_H
#define KLC3_SUBROUTINETRACKER_H

#include "klc3/FlowAnalysis/FlowGraph.h"
#include "klc3/Core/State.h"

namespace klc3 {

class SubroutineTracker {
public:

    SubroutineTracker(FlowGraph *fg, IssuePackage* issuePackage);

    void setupInitState(State *initState);

    void updateColors(State *state);  // make changes to state

    void postCheckState(State *state);

    void checkDuplicateColors();

    struct SubroutineInfo {
        string name;     // use first label as subroutine name
        Node *entry;
        uint16_t color;  // address of entry, used as color
        vector<Node *> exits;
        map<Edge *, vector<Edge *>> eePairs;  // entry-exit pairs
    };

    vector<SubroutineInfo> getSubroutines() const;

    uint16_t getSubroutineColor(const string &label) const;

    const SubroutineInfo &getSubroutineInfo(const string &label) const;

    const SubroutineInfo &getSubroutineInfo(uint16_t color) const;

    Node *getSubroutineEntry(const string &label) const;

    Subgraph getSubroutineSubgraph(const string &label) const;

private:

    FlowGraph *fg;
    IssuePackage *issuePackage;
    uint16_t initPC;

    void colorFlowGraph();

    void colorPath(Node *node, uint16_t color);

    void setUpEntryExitPairs(Edge *entryEdge);

    map<uint16_t , SubroutineInfo> subroutines;

    map<string, uint16_t> nameToColor;

    set<const Node *> checkedNodes;

    void checkDuplicateColorsOnPath(const Node *node, const set <uint16_t> &lastColors);

    string getLabelByAddr(uint16_t addr) const;

    /**
     * @note Coloring
     * We use color to reveal levels of subroutines in ideal execution case (not excluding unreachable code). A color
     * will propagate all the way down a path. Even in actual execution a color may not reach a node (not reachable
     * code or break in the middle), we still want to inform the student that the level can be improper.
     *
     * The color is represented using the starting value of the subroutine/main code, which can be used to easily
     * backtrack to the starting point of the subroutine.
     *
     * We use set<uint16_t> in Node to stores a list of colors as states go through them. We will check as last to see
     * whether a node has more than one color.
     *
     * After constructing the flow graph, we will color initially reachable code once starting at initPC. Although we
     * can try to color those initially unreachable code using the standalone starting point, but then we will need to
     * merge colors once they are reached by dynamic jumps or so, which complicate the problem. Instead, we will still
     * use updatePath() during execution-time update, so that once they are reached, the whole path will get colored
     * (which means that if there is a conflict, the conflict will propagate all the way down immediately).
     */

    /**
     * @note Color Stack and JSR stack
     * We keep track of JSR and RET through colorStack and jsrStack (storing the JSR node). When entering the main code
     * or a subroutine, its color is pushed onto the colorStack. When executing an JSR, the node is pushed onto the
     * jsrStack (therefore colorStack.size() should be equal to jsrStack.size() + 1 ideally). If an RET is proper, PC
     * should be at the next address after jsrStack.top(). If not, this RET is messing up colorStack and jsrStack, in
     * which case the flag stackHasMessedUp will be set, and a warning will be given. Since them, all RETs (including
     * the current one) of this state will be regarded as normal JMP which doesn't pop colorStack and jsrStack, and no
     * duplicate WARN_IMPROPER_RET warning will be given.
     */
};

}


#endif //KLC3_SUBROUTINETRACKER_H
