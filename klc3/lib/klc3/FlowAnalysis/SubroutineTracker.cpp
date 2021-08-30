//
// Created by liuzikai on 10/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/FlowAnalysis/SubroutineTracker.h"
#include "klc3/Generation/ReportFormatter.h"

namespace klc3 {

SubroutineTracker::SubroutineTracker(FlowGraph *fg, IssuePackage *issuePackage)
        : fg(fg), issuePackage(issuePackage), initPC(fg->getInitPC()) {

    colorFlowGraph();
}

void SubroutineTracker::colorFlowGraph() {
    // Only color initially reachable nodes
    Node *pcNode = fg->getNodeByAddr(initPC);
    if (pcNode) {

        subroutines[initPC] = {};
        SubroutineInfo *s = &subroutines[initPC];
        s->color = initPC;
        s->entry = pcNode;
        s->name = "(main)";
        nameToColor[s->name] = initPC;

        colorPath(pcNode, initPC);

        vector<Edge *> originalEdges = fg->allEdges();  // make a copy as new Edges are to constructed in the loop
        for (auto &edge : originalEdges) {
            if (edge->type() == Edge::JSR_EDGE || edge->type() == Edge::JSRR_EDGE) {
                if (subroutines.find(edge->to()->addr()) == subroutines.end()) {
                    newProgWarn() << "subroutine " << edge->to()->inst()->getLabel()
                                  << " is unreachable in static analysis\n";
                } else {
                    setUpEntryExitPairs(edge);
                }
            }
        }
    }
}

void SubroutineTracker::colorPath(Node *node, uint16_t color) {

    if (node->subroutineColors.find(color) != node->subroutineColors.end()) {  // this path have been colored
        return;
    }

    node->subroutineColors.insert(color);  // color the node

    // This function may be called during execution time, when dynamic edges may have been recognized
    if (node->inst()->instID() == InstValue::JSR || node->inst()->instID() == InstValue::JSRR) {

        for (auto &outEdge : node->runtimeOutEdges()) {
            if (outEdge->type() == Edge::JSR_EDGE || outEdge->type() == Edge::JSRR_EDGE) {
                Node *entryNode = outEdge->to();
                assert(entryNode != nullptr && "JSR/JSRR to nullptr node");

                // Create a new subroutine info structure

                SubroutineInfo t;
                t.name = entryNode->inst()->getLabel();
                t.entry = entryNode;
                t.color = entryNode->addr();
                subroutines[entryNode->addr()] = t;
                nameToColor[t.name] = t.color;

                entryNode->subroutineEntry = true;

                colorPath(entryNode, t.color);
            }
        }

        Node *retToNode = fg->getNodeByAddr(node->addr() + 1);  // get the node that RET is supposed to return to
        if (retToNode) {
            colorPath(retToNode, color);
        }

    } else if (node->inst()->isRET()) {

        if (color != initPC) {
            // Record exits first, construct entry-exit pairs later
            assert(subroutines.find(color) != subroutines.end());
            subroutines[color].exits.push_back(node);
        } else {
#if 1
            progInfo() << "Detect RET in (main) at " << node->inst()->sourceContext() << "\n";
#endif
        }

    } else {

        // Continue coloring the path
        for (auto *outEdge : node->runtimeOutEdges()) {
            if (outEdge->to() != nullptr) {
                colorPath(outEdge->to(), color);
            }
        }
    }
}

void SubroutineTracker::setUpEntryExitPairs(Edge *entryEdge) {
    assert(subroutines.find(entryEdge->to()->addr()) != subroutines.end() && "SubroutineInfo has not been set up yet");
    SubroutineInfo *s = &subroutines[entryEdge->to()->addr()];
    assert(s->eePairs.find(entryEdge) == s->eePairs.end() && "setUpEntryExitPairs for twice");
    s->eePairs[entryEdge] = {};
    auto &eePairs = s->eePairs[entryEdge];
    Node *retToNode = fg->getNodeByAddr(entryEdge->from()->addr() + 1);
    if (retToNode) {
        for (auto &exit : s->exits) {
            Edge *exitEdge = fg->newEdge(exit, retToNode, Edge::RET_EDGE);
            exitEdge->associatedEdge = entryEdge;
            eePairs.emplace_back(exitEdge);
            // No need to color the path since it will be colored from JSR/JSRR node
        }
    }
}

void SubroutineTracker::setupInitState(State *initState) {
    // Entering the main code
    initState->colorStack.push(initPC);  // use the starting addr of the main as color

    // Color the pcNode
    Node *pcNode = fg->getNodeByAddr(initPC);
    if (pcNode) pcNode->subroutineColors.insert(initState->colorStack.top());
}

void SubroutineTracker::updateColors(State *state) {

    if (state->latestNonOSInst[0].isNull()) return;

    Node *executedNode = state->latestNonOSInst[0]->node;
    Node *pcNode = fg->getNodeByAddr(state->getPC());

    if (pcNode == nullptr) {  // about to go into a virtual edge or about to fail, do nothing
        return;
    }

    InstValue::InstID executedInstID = executedNode->inst()->instID();

    if (executedInstID == InstValue::JSR || executedInstID == InstValue::JSRR) {

        uint16_t newColor = state->getPC();  // use the starting addr of the new subroutine as color

        if (subroutines.find(newColor) == subroutines.end()) {

            // Handle newly identified JSRR edge
            subroutines[newColor] = {};
            SubroutineInfo *t = &subroutines[newColor];
            t->name = pcNode->inst()->getLabel();
            t->entry = pcNode;
            t->color = newColor;

            pcNode->subroutineEntry = true;

            colorPath(pcNode, t->color);
            setUpEntryExitPairs(fg->getOnEdge(state));
        }

        state->colorStack.push(newColor);
        state->jsrStack.push(executedNode);

    } else if (executedInstID == InstValue::JMP && executedNode->inst()->baseR() == R_R7) {  // RET

        if (state->colorStack.size() >= 2) {  // must have at least two colors if it's a proper return

            if (state->stackHasMessedUp) {

                /*
                 * If state's stack has messed up, we regard this RET as normal JMP (no change to colorStack and
                 * jsrStack) and no further warnings will be produced.
                 */

            } else {

                assert(!state->jsrStack.empty() && "jsrStack should not be empty when state->colorStack.size() >= 2");

                if (state->jsrStack.top()->addr() == state->getPC() - 1) {
                    // RET to the proper location

                    state->colorStack.pop();
                    assert(state->jsrStack.top()->subroutineColors.find(state->colorStack.top()) !=
                           state->jsrStack.top()->subroutineColors.end() &&
                           "Color inconsistent");
                    state->jsrStack.pop();

                } else {

                    int poppedColor = state->colorStack.top();  // pop the top to get the second
                    if (auto issueInfo = state->newStateIssue(Issue::WARN_IMPROPER_RET, executedNode->inst())) {
                        issueInfo->setNote(
                                "expect RET to addr " + toLC3Hex(state->jsrStack.top()->addr() + 1) +
                                " right after " +
                                        reportFormatter->formattedContext(state->jsrStack.top()->inst()) +
                                ", but actually RET to addr " + toLC3Hex(state->getPC()) + ".\n");
                    }
                    // State status is set by newStateIssue
                    state->colorStack.push(poppedColor);  // recover the colorStack

                    Edge *onEdge = fg->getOnEdge(state);
                    if (onEdge) {
                        onEdge->isImproperRET = true;
                    }

                    state->stackHasMessedUp = true;
                }

            }

        } else {
            assert(!state->colorStack.empty() && "Why colorStack can be empty?");
            if (auto issueInfo = state->newStateIssue(Issue::ERR_RET_IN_MAIN_CODE, executedNode->inst())) {
                issueInfo->setNote("main code starts at: " + getLabelByAddr(state->colorStack.top()) + ".\n");
            }
            // By default ERROR, but can allow WARNING or NONE
            state->stackHasMessedUp = true;
        }

    }

    // Color the pcNode and following path, in any cases
    // This is used to introduce duplicate colors to detect shared code
    if (!state->stackHasMessedUp) {
        colorPath(pcNode, state->colorStack.top());
    }
}

void SubroutineTracker::checkDuplicateColors() {
    checkedNodes.clear();
    for (const auto &node : fg->allNodes()) {
        if (checkedNodes.find(node) == checkedNodes.end()) {
            checkDuplicateColorsOnPath(node, {});
        }
    }
}

void SubroutineTracker::checkDuplicateColorsOnPath(const Node *node, const set<uint16_t> &lastColors) {

    if (checkedNodes.find(node) != checkedNodes.end()) return;  // return if this path has already been checked

    checkedNodes.emplace(node);

    // Avoid producing duplicate warning on the same path
    if (node->subroutineColors != lastColors) {
        if (node->subroutineColors.size() > 1) {
            if (auto issueInfo = issuePackage->newGlobalIssue(Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES, node->inst())) {
                llvm::raw_string_ostream note(issueInfo->note);
                note << "Shared by: ";
                for (auto it = node->subroutineColors.begin(); it != node->subroutineColors.end(); ++it) {
                    if (it != node->subroutineColors.begin()) note << " & ";
                    note << getLabelByAddr(*it);
                }
                note << ".";
            }
            // State status is set by newStateIssue
        }
    }

    for (const auto &outEdge : node->runtimeOutEdges()) {
        if (outEdge->to()) {
            checkDuplicateColorsOnPath(outEdge->to(), node->subroutineColors);
        }
    }
}

string SubroutineTracker::getLabelByAddr(uint16_t addr) const {
    const ref<InstValue> &inst = fg->getNodeByAddr(addr)->inst();
    if (inst->labels.empty()) {
        if (addr == initPC) {
            return "(main)";
        } else {
            return toLC3Hex(inst->addr).c_str();
        }
    } else {
        return join("/", inst->labels);
    }
}

uint16_t SubroutineTracker::getSubroutineColor(const string &label) const {
    assert(nameToColor.find(label) != nameToColor.end() && "Unknown subroutine name");
    return nameToColor.find(label)->second;
}

Node *SubroutineTracker::getSubroutineEntry(const string &label) const {
    return getSubroutineInfo(label).entry;
}

Subgraph SubroutineTracker::getSubroutineSubgraph(const string &label) const {
    uint16_t subroutineColor = getSubroutineColor(label);
    Subgraph ret;
    for (const auto &node : fg->allNodes()) {
        if (node->subroutineColors.find(subroutineColor) != node->subroutineColors.end()) {
            ret.nodes.emplace(node);
        }
    }
    return ret;
}

const SubroutineTracker::SubroutineInfo &SubroutineTracker::getSubroutineInfo(uint16_t color) const {
    assert(subroutines.find(color) != subroutines.end() && "Unknown subroutine color");
    return subroutines.find(color)->second;
}

const SubroutineTracker::SubroutineInfo &SubroutineTracker::getSubroutineInfo(const string &label) const {
    return getSubroutineInfo(getSubroutineColor(label));
}

vector<SubroutineTracker::SubroutineInfo> SubroutineTracker::getSubroutines() const {
    vector<SubroutineInfo> ret;
    ret.reserve(subroutines.size());
    for (const auto &it : subroutines) {
        ret.push_back(it.second);
    }
    return ret;
}

void SubroutineTracker::postCheckState(State *s) {
    if (!s->stackHasMessedUp && s->colorStack.top() != fg->getInitPC()) {
        s->newStateIssue(Issue::WARN_HALT_IN_SUBROUTINE, s->latestNonOSInst[0]);
    }
    // State status is set by newStateIssue
}

}