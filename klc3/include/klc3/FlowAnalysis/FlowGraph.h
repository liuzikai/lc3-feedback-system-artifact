//
// Created by liuzikai on 4/10/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_FLOWGRAPH_H
#define KLC3_FLOWGRAPH_H

#include "klc3/FlowAnalysis/FlowGraphBasic.h"
#include "klc3/Core/MemoryValue.h"

namespace klc3 {

// Forward declared dependencies
class State;

class Loop;

/**
 * FlowGraph is directed graph. Each edge has an "from" Node and a "to" Node. Each Node records in edges and out edges.
 * Nodes and Edges are created and deleted with dynamic allocation, managed by FlowGraph.
 *
 * OS code are not encoded into the FlowGraph since basically we don't care about the code flow in the OS code. We
 * don't care about the code coverage of the OS code (if a TRAP is not used, whatever). TRAPs are expected implemented
 * correctly and KLC3 protects the OS code from being overwritten (?) so they always return or go to HALT. Out edges of
 * TRAPs are marked with TRAP_VIRTUAL_EDGE (explained below).
 *
 *
 *
 * Edges have many types. All types of Edges are contained in the same FlowGraph, providing a global views.
 *
 * Runtime Edge (with type() < Edge::TYPE_RUNTIME_COUNT) are edges that are derived from the program, which represent
 * actual control flow. Except TRAP_VIRTUAL_EDGE (explained below), other types of runtime edges represent ONE steps in
 * actual execution.
 *
 * JMP_EDGE is the out edge for JMP R0 to R6. JMP R7 is recognized as RET_EDGE. JSRR_EDGE is the out edge of JSRR. As
 * these two types of edges are only constructed at dynamic executions, their existence infers the actual control flow.
 *
 * RET_EDGE is the out edge for RET (JMP R7), connecting RET to the return location. One RET may have multiple RET_EDGE.
 * During the static analysis stage, inferred RET_EDGEs are constructed for each JSR call (JSRR won't be determined at
 * static analysis), assuming R7 is correctly set. The associatedEdge field of a RET_EDGE is the corresponding JSR Edge.
 * At the dynamic execution stage, if R7 is indeed correctly set, the RET_EDGE will be covered (see below). However, if
 * R7 doesn't match the expected value, a new RET_EDGE will be constructed based on the actual R7 value, in which case
 * the associatedEdge is not set.
 *
 * Since RET_EDGE is constructed in both static analysis and dynamic execution stage, the existence of a RET_EDGE
 * doesn't necessarily infer the actual control flow. Check for its coverage (see below).
 *
 * TRAP out edges have type TRAP_VIRTUAL_EDGE. The other runtime edges are marked as NORMAL_EDGE.
 *
 * TRAP out edges have the special type of Edge::TRAP_VIRTUAL_EDGE. Rather than connecting to the node in the OS code
 * (exactly next step at execution), a TRAP_VIRTUAL_EDGE goes to the node of the next instruction in the user code,
 * which is the next-line instruction for TRAP other than HALT and nullptr for HALT. It's designed in this way because
 * we basically don't want to keep track of flow in the OS code and the TRAPs are expected to be implemented correctly.
 *
 * An INIT_PC_ENTRY_EDGE is a hypothetical edge from nullptr to the node pointed by initial PC. It is designed for
 * consistent definitions of "a state is 'on' an edge" and "an edge is covered" (both explained below).
 *
 *
 *
 * Non-runtime edges are not derived from the program, including SUBROUTINE_VIRTUAL_EDGE, LOOP_H2H_EDGE and
 * LOOP_H2X_EDGE.
 *
 * A SUBROUTINE_VIRTUAL_EDGE is a hypothetical edge connected from a JSR or JSRR to its next-line instruction. That is,
 * it assumes the subroutine call will return correctly. It's used in PruningSearcher which has special handles for
 * loop segments containing subroutine calls.
 *
 * LOOP_H2H_EDGEs and LOOP_H2X_EDGEs are hypothetical edges constructed with LoopAnalyzer. See descriptions there.
 *
 *
 *
 * Each edge has an "from" Node and a "to" Node. The from Node is nullptr only for INIT_PC_ENTRY_EDGE. The to Node is
 * nullptr only for TRAP_VIRTUAL_EDGE of HALT.
 *
 * A state is said to be "on" an edge if the from Node is executed and the to Node is to be executed. For runtime edges
 * except TRAP_VIRTUAL_EDGE, the to node is exactly points by PC. FlowGraph::getOnEdge() is used to get the "on" edge
 * of a state, which returns only runtime edge. When a State is inside the OS code, executing a TRAP,
 * FlowGraph::getOnEdge() returns the TRAP_VIRTUAL_EDGE. Before executing the first instruction, a state is "on" the
 * INIT_PC_ENTRY_EDGE.
 *
 * An edge is said to be "covered" if both the from Node and the to Node are executed successfully (not BROKEN). For a
 * INIT_PC_ENTRY_EDGE, it is covered when the first instruction is executed successfully. A TRAP_VIRTUAL_EDGE is
 * connected from a TRAP to its next-line instruction, so it's covered when the next-line instruction is executed
 * successfully. FlowGraph::getJustCoveredEdge() returns the just covered edge in the last step. For a
 * TRAP_VIRTUAL_EDGE, it returns nullptr until the next-line instruction of the TRAP is executed.
 *
 *
 * Since all types of edges are stored in the same FlowGraph, types should be carefully checked when iterating edges.
 *
 *
 * Since the FlowGraph is used everywhere in KLC3, we must be careful about the access control. Node and Edge mainly
 * use getter and class access control. Constructors and internal variables are protected and friended to FlowGraph.
 * Node and Edge also contains variables used by modules, which are merely public as their usage are usually limited
 * with in that module. Access control is better than passing const pointers around as there is no easy way to make all
 * pointers const recursively. Also, there is no easy way to cast vector<Edge *> to const vector<const Edge*> &.
 */

class Node {
public:

    const string &name() const { return name_; }
    int addr() const { return addr_; }
    const ref<InstValue> &inst() const { return inst_; }

    const vector<Edge *> &runtimeOutEdges() const { return runtimeOutEdges_; }
    const vector<Edge *> &allOutEdges() const { return allOutEdges_; }
    Edge *uniqueRuntimeOutEdge() const { return uniqueRuntimeOutEdge_; }

    const vector<Edge *> &runtimeInEdges() const { return runtimeInEdges_; }
    const vector<Edge *> &allInEdges() const { return allInEdges_; }

    string context() const { return inst().isNull() ? "(null)" : inst()->sourceContext(); }

    void dump() const { progErrs() << context(); }

    // ================ Filled by SubroutineTracker ================

    bool subroutineEntry = false;
    bool possibleRETPoint = false;
    set <uint16_t> subroutineColors;  // used for JSR(R) tracking, using JSR staring point addr as index

    // ================ Filled by LoopAnalyzer ================

protected:

    friend class FlowGraph;

    Node() = default;  // constructor only open to FlowGraph

    string name_;
    int addr_;
    ref <InstValue> inst_;

    // Since only pointers are stored and managed by Flowgraph, just make two copies
    vector<Edge *> runtimeOutEdges_;
    vector<Edge *> allOutEdges_;
    Edge *uniqueRuntimeOutEdge_ = nullptr;

    vector<Edge *> runtimeInEdges_;
    vector<Edge *> allInEdges_;

    unsigned index;  // for internal use of FlowGraph, to accelerate dist calculation

};

class Edge {
public:

    enum Type {
        NORMAL_EDGE,  // certain both static and dynamic, single step

        JMP_EDGE,     // constructed at dynamic execution, single step, JMP R0 to R6. JMP R7 is recognized as RET_EDGE/
        JSRR_EDGE,    // constructed at dynamic execution, single step

        JSR_EDGE,     // certain both static and dynamic stage, single step
        RET_EDGE,     // potential, constructed at static stage, single step

        TRAP_VIRTUAL_EDGE,  // potential (unless OS gets override), statically constructed, multi-step, may lead to null

        INIT_PC_ENTRY_EDGE,

        TYPE_RUNTIME_COUNT,  // ================ all types above are of exactly runtime in user space ================

        SUBROUTINE_VIRTUAL_EDGE,  // potential (if return properly), statically constructed, multi-step

        LOOP_H2H_EDGE,  // abstract representation
        LOOP_H2X_EDGE,  // abstract representation

        // Be careful to handle known types in edge iterations everywhere
    };

    Type type() const { return type_; }

    bool isAcrossSubroutine() const { return type() >= JSRR_EDGE && type() <= RET_EDGE; }

    Node *from() const { return from_; };
    Node *to() const { return to_; };

    string fromContext() const { return from() ? from()->context() : "<none>"; }

    string toContext() const { return to() ? to()->context() : "<none>"; }

    enum Flags {
        BR_CONTINUE_EDGE = 1,
        BR_BRANCH_EDGE = 2,
        LIKELY_UNCOVERABLE = 4
    };
    unsigned flags = 0;  // allow public access

    /**
     * Guiding edge is a edge necessary to reconstructed a path:
     *  If an edge is the unique runtime out edge of its from node, it can be omitted. When reconstructing the path,
     *      this unique runtime out edge must be chosen by default if no other non-runtime edge is specified.
     *  If an edge is one of the runtime out edges of its from node, it must not be omitted.
     *  If an edge is a non-runtime edge, it must not be omitted.
     *  If an edge is from JMP(RET) or JSRR, it must not be omitted.
     *
     * This function check the number of out edges of the from node, so you should not call it after before all runtime
     * edges are constructed. Dynamically constructed edges are taken into considerations.
     *
     * @return Whether the current edge is a guiding edge
     */
    bool isGuidingEdge() const {
        if (type() > TYPE_RUNTIME_COUNT) return true;                // include non-runtime edge
        if (from()->uniqueRuntimeOutEdge() == nullptr) return true;  // include multiple choices
        if (!from()->inst().isNull()) {
            // Include dynamic instructions
            switch (from()->inst()->instID()) {
                case InstValue::JMP:
                case InstValue::JSRR:
                    return true;
                default:
                    break;
            }
        }
        return false;
    }

    // ================ Filled by CoverageTracker ================

    bool covered = false;  // handled in centralized way by CoverageTracker for easy coverage calculation

    // ================ Filled by SubroutineTracker ================

    bool isImproperRET = false;
    Edge *associatedEdge = nullptr;  // for RET_EDGE, the associated JSR_EDGE.

    // ================ Filled by PostPoneManager ================

    bool isFrontier = false;
    int postponeStateCount = 0;

protected:

    friend class FlowGraph;

    Type type_;
    Node *from_;
    Node *to_;

    Edge() = default;  // constructor only open to FlowGraph

};

class Subgraph {
public:
    unordered_set<Node *> nodes;

    bool containNode(Node *node) const { return nodes.find(node) != nodes.end(); }
};

class FlowGraph {
public:

    FlowGraph(const map <uint16_t, ref<MemValue>> &initMem, uint16_t initPC) : initPC(initPC) {
        memset(nodeByAddr, 0, sizeof(nodeByAddr));
        constructFromInitMem(initMem);
        constructInitPCEdge(initPC);
        if (!haveJMPOrJSRR) {
            trimBREdges();
        } else {
            newProgWarn() << "JMP or JSRR detected. BR edge trimming not performed.\n";
        }
    }

    ~FlowGraph();

    const vector<Node *> &allNodes() const { return nodes; }

    const vector<Edge *> &allEdges() const { return edges; }  // be sure to check the types of each edge.

    Node *newNode(int addr, const ref <InstValue> &inst);

    Edge *newEdge(Node *from, Node *to, Edge::Type type);

    /*
     * In some case such as matching node for PC, we still need this. In other cases, for example when you can get
     * uniquely associated node from InstValue, do not use this.
     */
    Node *getNodeByAddr(int addr) const;

    uint16_t getInitPC() const { return initPC; }

    Edge *getInitPCEdge() const { return initPCEdge; }

    Subgraph getFullGraph() const;

    /// ================================ Dynamic Update and Query ================================

    /**
     * Get the edge the state is "on" (.from == last executed edge, .to == PC), including identifying virtual edge.
     * Possibly return nullptr, if the edge doesn't exist in flow graph, in which case the state is expected to failed
     * as it step once more.
     * @param state
     * @return The edge the state is "on", may be nullptr.
     */
    Edge *getOnEdge(const State *state) const;

    /**
     * Get the edge that the state covered (both .from and .to are executed successfully). When the state is in virtual
     * edge or the state broke, return nullptr.
     * @param state
     * @return The edge that is just covered. nullptr if the state is still in virtual edge or the state broke.
     */
    Edge *getJustCoveredEdge(const State *state) const;

    using newEdgeCallback = void (*)(Edge *edge, void *arg);

    void registerNewEdgeCallback(newEdgeCallback callback, void *arg) { newEdgeCallbacks.emplace_back(callback, arg); }

private:

    static constexpr unsigned MAX_NODE_COUNT = 10000;

    vector<Node *> nodes;
    vector<Edge *> edges;

    uint16_t initPC;
    Edge *initPCEdge = nullptr;

    Node *nodeByAddr[0x10000];  // see above getNodeByAddr()

    bool haveJMPOrJSRR = false;

    void constructFromInitMem(const map <uint16_t, ref<MemValue>> &initMem);

    Edge *constructInitPCEdge(uint16_t initPC);

    void setNodeOutEdgesWithInst(const ref <InstValue> &inst);  // node must have been set up in inst->node

    Edge *newEdgeToAddrIfExists(Node *node, uint16_t toAddr, Edge::Type type, const string &fromContext);

    void trimBREdges();

    unsigned getEdgeCoverageCC(const Edge *edge, int depth);

    vector <pair<newEdgeCallback, void *>> newEdgeCallbacks;

};

}


#endif //KLEE_FLOWGRAPH_H
