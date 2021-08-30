//
// Created by liuzikai on 4/10/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/FlowAnalysis/FlowGraph.h"
#include "klc3/Core/State.h"

namespace klc3 {

FlowGraph::~FlowGraph() {
    for (auto &item : nodes) delete item;
    for (auto &item : edges) delete item;
}

Node *FlowGraph::newNode(int addr, const ref<InstValue> &inst) {
    assert (!inst.isNull() && "Node inst must not be null");
    assert (inst->addr == addr && "Node inst must have consistent address");
    if (nodeByAddr[addr] != nullptr) {
        newProgWarn() << "FlowGraph reports overwriting node at address " << toLC3Hex(addr) << ". "
                      << "The original node still exists but not accessible by getNodeByAddr()";
    }
    Node *node = new Node;
    node->name_ = inst->getLabel();
    node->addr_ = addr;
    node->inst_ = inst;
    node->index = nodes.size();
    assert(node->index < MAX_NODE_COUNT && "Too many nodes! Increase MAX_NODE_COUNT!");
    nodes.push_back(node);
    nodeByAddr[addr] = node;
    if (!inst.isNull()) inst->node = node;
    return node;
}

Edge *FlowGraph::newEdge(Node *from, Node *to, Edge::Type type) {
    Edge *edge = new Edge;
    edge->type_ = type;
    edge->from_ = from;
    edge->to_ = to;
    edges.emplace_back(edge);
    if (from != nullptr) {
        if(type < Edge::TYPE_RUNTIME_COUNT) from->runtimeOutEdges_.emplace_back(edge);
        from->allOutEdges_.emplace_back(edge);

        // Update uniqueRuntimeOutEdge
        from->uniqueRuntimeOutEdge_ = nullptr;
        for (auto &e : from->runtimeOutEdges_) {
            if (from->uniqueRuntimeOutEdge_ == nullptr) {
                // First runtime edge
                from->uniqueRuntimeOutEdge_ = e;
            } else {
                // Second runtime edge
                from->uniqueRuntimeOutEdge_ = nullptr;
                break;
            }
        }
    }
    if (to != nullptr) {
        if (type < Edge::TYPE_RUNTIME_COUNT) to->runtimeInEdges_.emplace_back(edge);
        to->allInEdges_.emplace_back(edge);
    }
    for (auto &item : newEdgeCallbacks) {
        item.first(edge, item.second);
    }
    return edge;
}

Node *FlowGraph::getNodeByAddr(int addr) const {
    return nodeByAddr[addr];
}

void FlowGraph::constructFromInitMem(const map<uint16_t, ref<MemValue>> &initMem) {
    // First pass: construct all nodes
    for (auto &item : initMem) {
        if (item.second->type == MemValue::MEM_INST && !item.second->belongsToOS) {
            uint16_t addr = item.first;
            ref<InstValue> inst = dyn_cast<InstValue>(item.second);
            assert(!inst.isNull());
            newNode(addr, inst);
        }
    }

    // Second pass: construct all edges and setup connection, including an edge from addr 0x0 to initPC
    for (auto &it : initMem) {
        if (it.second->type == MemValue::MEM_INST && !it.second->belongsToOS) {
            setNodeOutEdgesWithInst(dyn_cast<InstValue>(it.second));
        }
    }
}

Edge *FlowGraph::constructInitPCEdge(uint16_t initPC) {
    if (getNodeByAddr(initPC) == nullptr) {
        newProgWarn() << "Init PC doesn't match any node!\n";
        return nullptr;
    }
    initPCEdge = newEdge(nullptr, getNodeByAddr(initPC), Edge::INIT_PC_ENTRY_EDGE);
    return initPCEdge;
}

void FlowGraph::setNodeOutEdgesWithInst(const ref<InstValue> &inst) {
    uint16_t addr = inst->addr;
    uint16_t pc = addr + 1;
    Node *node = inst->node;

    if (inst->instID() == InstValue::BR) {

        int continueAddr = -1;
        int branchAddr = -1;
        if (inst->cc() != InstValue::CondCode::CC_NZP) {  // not always branch
            continueAddr = pc;
        }
        if (inst->cc() != InstValue::CondCode::CC_NONE) {  // not never branch
            branchAddr = (pc + inst->imm9()) & 0xFFFFU;
        }

        Edge *e;
        if (continueAddr != -1 && branchAddr != -1) {
            if (continueAddr == branchAddr) {
                e = newEdgeToAddrIfExists(node, continueAddr, Edge::NORMAL_EDGE, inst->sourceContext());
                if (e) e->flags |= Edge::BR_CONTINUE_EDGE | Edge::BR_BRANCH_EDGE;
            } else {
                e = newEdgeToAddrIfExists(node, continueAddr, Edge::NORMAL_EDGE, inst->sourceContext());
                if (e) e->flags |= Edge::BR_CONTINUE_EDGE;
                e = newEdgeToAddrIfExists(node, branchAddr, Edge::NORMAL_EDGE, inst->sourceContext());
                if (e) e->flags |= Edge::BR_BRANCH_EDGE;
            }
        } else if (continueAddr != -1) {
            e = newEdgeToAddrIfExists(node, continueAddr, Edge::NORMAL_EDGE, inst->sourceContext());
            if (e) e->flags |= Edge::BR_CONTINUE_EDGE | Edge::BR_BRANCH_EDGE;
        } else if (branchAddr != -1) {
            e = newEdgeToAddrIfExists(node, branchAddr, Edge::NORMAL_EDGE, inst->sourceContext());
            if (e) e->flags |= Edge::BR_CONTINUE_EDGE | Edge::BR_BRANCH_EDGE;
        } else
            assert(!"Impossible case");

    } else if (inst->instID() == InstValue::JSR) {

        // Continue on a JSR instruction should go to the address it points
        newEdgeToAddrIfExists(node, pc + inst->imm11(), Edge::JSR_EDGE, inst->sourceContext());

        // Construct a virtual edge to PC assuming JSR returns
        newEdgeToAddrIfExists(node, pc, Edge::SUBROUTINE_VIRTUAL_EDGE, inst->sourceContext());

    } else if (inst->instID() == InstValue::JMP || inst->instID() == InstValue::JSRR) {

        // Jump to dynamic address, cannot construct any edge with static analysis
        // When calculating distances, these nodes can reach any other node with DEFAULT_DYNAMIC_DIST step

        if (!inst->isRET()) haveJMPOrJSRR = true;

    } else if (inst->instID() == InstValue::TRAP) {

        if (inst->vec8() == InstValue::VEC8_GETC || inst->vec8() == InstValue::VEC8_OUT ||
            inst->vec8() == InstValue::VEC8_PUTS || inst->vec8() == InstValue::VEC8_IN ||
            inst->vec8() == InstValue::VEC8_PUTSP ||
            inst->vec8() == InstValue::VEC8_KLC3_COMMAND) {  // these traps comes back to next instruction
            // For now VEC8_KLC3_COMMAND is only used for customized warning, which will not halt the program

            newEdgeToAddrIfExists(node, pc, Edge::TRAP_VIRTUAL_EDGE, inst->sourceContext());

        } else {  // HALT and BAD_TRAP

            // Construct a virtual edge to nullptr
            newEdge(node, nullptr, Edge::TRAP_VIRTUAL_EDGE);

        }

    } else {

        // For other instructions, continue leads to next instruction
        // We have handle the cases of branching and jumping. So it's abnormal for the other normal instruction
        // to not have a node at next address
        newEdgeToAddrIfExists(node, pc, Edge::NORMAL_EDGE, inst->sourceContext());

    }

    if (inst->instID() == InstValue::JSR || inst->instID() == InstValue::JSRR) {
        Node *possibleRETNode = getNodeByAddr(pc);
        if (possibleRETNode) possibleRETNode->possibleRETPoint = true;
    }
}

Edge *FlowGraph::newEdgeToAddrIfExists(Node *node, uint16_t toAddr, Edge::Type type, const string &fromContext) {
    Node *toNode = getNodeByAddr(toAddr);
    if (toNode == nullptr) {
        newProgWarn() << "Detect an edge to an non-existing node." << "\n"
                      << "    From: " << fromContext << "\n"
                      << "    To addr: " << toLC3Hex(toAddr) << "\n";
        return nullptr;
    } else return newEdge(node, toNode, type);
}

void FlowGraph::trimBREdges() {
    vector<Edge *> edgesToTrim;
    for (auto &e : edges) {
        if (e->type() < Edge::TYPE_RUNTIME_COUNT) {
            if (e->from() && e->from()->inst()->instID() == InstValue::BR) {

                // Never trim an always edge
                if ((e->flags & Edge::BR_CONTINUE_EDGE) && (e->flags & Edge::BR_BRANCH_EDGE)) continue;


                if (getEdgeCoverageCC(e, 10) == InstValue::CC_NONE) {
                    edgesToTrim.emplace_back(e);
                }
            }
        }
    }
    for (auto &e : edgesToTrim) {
        timedInfo() << "Trim the edge from " << e->fromContext() << " -> " << e->toContext() << "\n";
        e->flags |= Edge::LIKELY_UNCOVERABLE;
    }
}


unsigned FlowGraph::getEdgeCoverageCC(const Edge *edge, int depth) {
    if (depth == 0) {
        newProgWarn() << "getNodeCoverageCC reaches depth limit\n";
        return InstValue::CC_NZP;
    }

    const Node *fromNode = edge->from();

    if (fromNode == nullptr) return InstValue::CC_NZP;

    // Just don't border to check for loops, use depth for limitation

    unsigned ret = 0;
    switch (fromNode->inst()->instID()) {
        case InstValue::JMP:
        case InstValue::JSR:
        case InstValue::JSRR:
        case InstValue::ST:
        case InstValue::STI:
        case InstValue::STR:
        case InstValue::TRAP:
        case InstValue::RTI:
        case InstValue::BR:
            // These instructions doesn't change CC
            for (const auto &e : fromNode->allInEdges()) {
                if (e->type() == Edge::TRAP_VIRTUAL_EDGE || e->type() == Edge::SUBROUTINE_VIRTUAL_EDGE) {
                    ret = InstValue::CC_NZP;
                } else if (e->type() < Edge::TYPE_RUNTIME_COUNT) {
                    ret |= getEdgeCoverageCC(e, depth - 1);
                }
            }
            if (fromNode->inst()->instID() == InstValue::BR) {
                unsigned brCC = fromNode->inst()->cc();
                if ((edge->flags & Edge::BR_CONTINUE_EDGE) && (edge->flags & Edge::BR_BRANCH_EDGE) == 0) {
                    // Continue edge, reverse the condition
                    brCC = 7 ^ brCC;
                }
                ret &= brCC;  // apply the mask
            }
            break;
        default:
            // The other instruction changes CC
            ret = InstValue::CC_NZP;
            break;
    }
    return ret;
}

Edge *FlowGraph::getOnEdge(const State *state) const {
    if (state->latestNonOSInst[0].isNull()) return initPCEdge;
    Node *fromNode = state->latestNonOSInst[0]->node;

    // Look for virtual edge
    for (auto &e: fromNode->runtimeOutEdges()) {
        if (e->type() == Edge::TRAP_VIRTUAL_EDGE) {
            return e;
        }
    }

    // Look for out edge
    Node *toNode = getNodeByAddr(state->getPC());
    if (toNode == nullptr) return nullptr;
    for (auto &edge : fromNode->runtimeOutEdges()) {
        if (edge->to() == toNode) return edge;
    }

    // No on edge, return nullptr, and the state is expected to break next step!
    return nullptr;
}

Edge *FlowGraph::getJustCoveredEdge(const State *state) const {

    if (state->status == State::HALTED) {

        // Find and return the virtual edge to halt
        for (auto &e : state->latestNonOSInst[0]->node->runtimeOutEdges()) {
            if (e->type() == Edge::TRAP_VIRTUAL_EDGE && e->to() == nullptr) return e;
        }

        // Fail to find virtual edge to halt, unexpected halt
        return nullptr;

    } else if (state->status == State::NORMAL) {

        if (state->latestNonOSInst[0].isNull()) return nullptr;

        if (state->latestNonOSInst[0].get() != state->latestInst[0].get()) {

            /*
             * Still in virtual edge.
             * Notice that you should not use state->lastNthFetchedNonOSInst[0]->instID() == InstValue::TRAP. When
             * the node is just on the first step to OS node (PC points to virtual node, but not yet executed), this
             * function needs to correctly recognize the last edge in user space.
             */

            return nullptr;

        } else {

            if (state->latestNonOSInst[1].isNull()) return initPCEdge;  // starting edge
            for (auto &e : state->latestNonOSInst[1]->node->runtimeOutEdges()) {
                if (e->to() == state->latestNonOSInst[0]->node) {
                    return e;  // may be a normal edge or a virtual edge
                }
            }

        }
        assert(!"Fail to match covered edge");

    } else return nullptr;
}

Subgraph FlowGraph::getFullGraph() const {
    Subgraph fullGraph;
    for (auto &node : allNodes()) {
        fullGraph.nodes.emplace(node);
    }
    return fullGraph;
}

Node *Path::src() const {
    return es.empty() ? nullptr : es.front()->from();
}

Node *Path::dest() const {
    return es.empty() ? nullptr : es.back()->to();
}

void Path::append(Edge *edge) {
#if 0
    // Sanity check on whether the path can be reconstructed
    if (!es.empty()) {
        Node *node = es.back()->to;
        while (node != edge->from) {
            assert(node->uniqueRuntimeOutEdge && "Deadend or multiple choices");
            node = node->uniqueRuntimeOutEdge->to;
        }
        // Reached edge->from, check passed
    }
#endif
    es.emplace_back(edge);
}

void Path::compress() {
    if (es.empty()) return;
    vector<Edge *> res;
    for (auto &e : es) {
        if (e == es.front() || e == es.back() || e->isGuidingEdge()) {
            res.emplace_back(e);
        }
    }
    es = std::move(res);
}

bool Path::appendCompressed(Edge *edge, bool isLast) {
    if (isLast || es.empty() || edge->isGuidingEdge()) {
        es.emplace_back(edge);
        return true;
    }
    return false;
}

vector<Edge *> Path::reconstructedFullPath() const {
    if (es.empty()) {
        return {};
    }

    vector<Edge *> res = {es.front()};
    Node *curNode = es.front()->to();
    auto it = es.cbegin() + 1;  // next guiding edge iterator
    while(it != es.cend()) {
        Edge *target = nullptr;
        assert(curNode && "Dead end");
        for (auto &e : curNode->allOutEdges()) {  // not limited to runtime out edges
            if (e == *it) {
                target = e;
                break;
            }
        }
        if (target != nullptr) {
            // Always consider the guiding edge first
            // Match guiding edge, move to next guiding edge
            ++it;
        } else {
            if (curNode->uniqueRuntimeOutEdge()) {
                // If there is a unique runtime out edge, must use it first
                target = curNode->uniqueRuntimeOutEdge();
            } else {
                assert(!"Can not reconstruct the path");
            }
        }
        res.emplace_back(target);
        curNode = target->to();
    }
    return res;
}

void Path::dump() const {
    if (es.empty()) progErrs() << "(empty)\n";
    Node *lastNode = es.front()->from();
    lastNode->inst()->dump();
    for (auto edge : es) {
        progErrs() << "... -> ";
        edge->to()->inst()->dump();
        lastNode = edge->to();
    }
}

} // namespace klc3
