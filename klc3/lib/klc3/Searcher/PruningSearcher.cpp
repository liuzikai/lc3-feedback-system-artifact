//
// Created by liuzikai on 11/16/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Searcher/PruningSearcher.h"

#define PRUNING_SEARCHER_ECHO_POSTPONE  0
#define PRUNING_SEARCHER_ECHO_REACTIVATION  0
#define PRUNING_SEARCHER_ECHO_SEGMENT_COVERAGE  1

namespace klc3 {


PruningSearcher::PruningSearcher(const FlowGraph *fg, Searcher *internalSearcher,
                                 const set<Loop *> &topLevelLoops, const set<Loop *> &allLoops)
        : fg(fg), internalSearcher(internalSearcher), topLevelLoops(topLevelLoops) {

    for (auto &loop : allLoops) {
        uncoveredLoops[loop] = LoopInfo(loop);
    }
    time_t seed = std::time(nullptr);
    progInfo() << "PruningSearcher: seed " << seed << "\n";
    std::srand(seed);
}

void PruningSearcher::push(const StateVector &states) {

    StateVector statesToGoThrough;

    for (auto &s : states) {

        if (s->status == State::BROKEN || s->stackHasMessedUp) {
            // Bypass BROKEN states or messed up state
            statesToGoThrough.push_back(s);
            continue;
        }
        // Process NORMAL or HALTED states

        if (fg->getJustCoveredEdge(s) == nullptr) {
            // Initial state or on virtual edge (onEdge will always be the virtual edge)
            statesToGoThrough.push_back(s);
            continue;
        }

        Edge *onEdge = fg->getOnEdge(s);
        if (onEdge == nullptr) {
            // Ready to break next step, keep going
            statesToGoThrough.push_back(s);
            continue;
        }

        if (!s->loopStack.empty()) {
            // Append onEdge to the loopPathFromHead of the top-level loop
            // The inner most loop must deal with runtime edge
            appendLoopPath(s, onEdge);
        }

        // Check for loop exit and H2X segment coverage
        testLoopExitAndUpdateH2XCoverage(s, onEdge);

        // Enter new loop if any
        enterNewLoopIfAny(s, onEdge);

        // When reaching the head again, reset pathFromLoopEntry and H2H coverage
        testH2HAndUpdate(s, onEdge);

        if (s->coveredNewEdge || s->coveredNewSegment || s->avoidLoopReductionPostpone) {
            // Never postpone a state that has cover new edge (code coverage) or new segment (loop coverage)
            statesToGoThrough.push_back(s);
            continue;
        }


        if (!s->latestNonOSInst[0].isNull() && s->latestNonOSInst[0]->instID() == InstValue::BR) {
            // Only consider postponing NEWLY created states at BR

            // This is the only chance that a state can get postponed
            if (s->loopStack.empty() || onPrefixOfUncoveredSegment(s)) {
                statesToGoThrough.push_back(s);
                s->avoidLoopReductionPostpone = true;  // it will never get postponed later
            } else {
                postponedStates.push_back(s);
                statPostponeCount++;
#if PRUNING_SEARCHER_ECHO_POSTPONE
                if (onEdge->to) progInfo() << "Postpone S" << s->getUID() << " at " << onEdge->to->inst->sourceContext() << "\n";
#endif
            }
        } else {
            s->avoidLoopReductionPostpone = true;  // it will never get postponed later
            statesToGoThrough.push_back(s);
            continue;
        }
    }

    if (!statesToGoThrough.empty()) {
        internalSearcher->push(statesToGoThrough);
    }
}

void PruningSearcher::appendLoopPath(State *s, Edge *onEdge) const {
    assert(!s->loopStack.empty());
    auto &topLoop = s->loopStack.back();

    // If the state just goes to another subroutine, ignore
    if (s->colorStack.top() == topLoop.loopColor) {

        // If just returned to the subroutine where the top level loop locates, append the subroutine virtual edge
        if (onEdge->type() == Edge::RET_EDGE) {
            for (auto &edge : onEdge->to()->allInEdges()) {
                if (edge->type() == Edge::SUBROUTINE_VIRTUAL_EDGE) {

                    topLoop.loopPathFromEntry.appendCompressed(edge, topLoop.loop->segmentLastEdges().find(edge) !=
                                                               topLoop.loop->segmentLastEdges().end());
                    return;
                }
            }
            assert(!"Failed to find the SUBROUTINE_VIRTUAL_EDGE");
        } else {
            // Deal with runtime edge
            topLoop.loopPathFromEntry.appendCompressed(onEdge, topLoop.loop->segmentLastEdges().find(onEdge) !=
                                                               topLoop.loop->segmentLastEdges().end());
        }
    }
}

bool PruningSearcher::insideTopLoop(State *s, Edge *onEdge) {
    Node *lastNode = onEdge->to();
    auto &topLoop = s->loopStack.back();

    // If the state is still inside a subroutine, ignore
    if (s->colorStack.top() != topLoop.loopColor) {
        // Go to a different subroutine

        // If just covered and RET edge, not inside the loop any more
        if (onEdge->type() == Edge::RET_EDGE &&
            !onEdge->from()->subroutineColors.empty() &&
            *onEdge->from()->subroutineColors.begin() == topLoop.loopColor) {

            return false;
        }

        // Otherwise, entering/within another subroutine
        return true;
    }

    return (topLoop.loop->nodes().find(lastNode) != topLoop.loop->nodes().end());
}

void PruningSearcher::testLoopExitAndUpdateH2XCoverage(State *s, Edge *onEdge) {
    Node *lastNode = onEdge->to();
    while (!s->loopStack.empty() && !insideTopLoop(s, onEdge)) {

        Loop *exitLoop = s->loopStack.back().loop;
        Path &h2x = s->loopStack.back().loopPathFromEntry;

        // Assert using value comparison on Path
        assert(exitLoop->h2xSegments().find(h2x) != exitLoop->h2xSegments().end() &&
               "Unmatched recorded h2x segment");

        // Update global coverage
        if (uncoveredLoops.find(exitLoop) != uncoveredLoops.end()) {
            LoopInfo &info = uncoveredLoops[exitLoop];
            auto it = info.uncoveredH2XSegments.find(h2x);
            if (it != info.uncoveredH2XSegments.end()) {
#if PRUNING_SEARCHER_ECHO_SEGMENT_COVERAGE
                timedInfo() << "PruningSearcher: " << it->tag << " in " << exitLoop->name() << " covered by S"
                            << s->getUID() << ", " << info.uncoveredH2XSegments.size() - 1 << " left: ";
                for (auto it2 = info.uncoveredH2XSegments.begin(); it2 != info.uncoveredH2XSegments.end(); ++it2) {
                    if (it2 != it) {
                        progInfo() << it2->tag << " ";  // no time stamp
                    }
                }
                progInfo() << "\n";
//                h2x.dump();
#endif
                info.uncoveredH2XSegments.erase(it);
                s->coveredNewSegment = true;
            }
            if (info.uncoveredH2HSegments.empty() && info.uncoveredH2XSegments.empty()) {
                // The loop is fully covered
                uncoveredLoops.erase(exitLoop);
#if PRUNING_SEARCHER_ECHO_SEGMENT_COVERAGE
                timedInfo() << "PruningSearcher: " << exitLoop->name() << " is fully covered" << "\n";
#endif
            }
        }

        // Exit the loop
        s->loopStack.pop_back();

        if (!s->loopStack.empty() && s->colorStack.top() == s->loopStack.back().loopColor) {
            // Update upper level loop with h2xEdge of the just exited loop

            State::LoopLayer &upperLoopLayer = s->loopStack.back();
            Path &upperPath = upperLoopLayer.loopPathFromEntry;
            assert(!upperPath.empty() && "Empty upperPath implies entering multiple levels at the same points?");

            // Find the h2xEdge of the just exited loop using existing path info
            Edge *subloopH2XEdge = nullptr;
            for (auto &edge : exitLoop->h2xEdges()) {
                // edge->from may not be the same due to reduced edge storage
                if (edge->to() == lastNode) {
                    subloopH2XEdge = edge;
                    break;
                }
            }
            assert(subloopH2XEdge && "Failed to find h2xEdge of the just exited loop");
            upperPath.appendCompressed(subloopH2XEdge, upperLoopLayer.loop->segmentLastEdges().find(subloopH2XEdge) !=
                                                       upperLoopLayer.loop->segmentLastEdges().end());
            // Continue to check further exit
        }
    }
}

void PruningSearcher::enterNewLoopIfAny(State *s, Edge *onEdge) const {
    Loop *enteringLoop;
    if (s->loopStack.empty() || s->colorStack.top() != s->loopStack.back().loopColor) {
        enteringLoop = findLoop(topLevelLoops, onEdge->to());
    } else {
        enteringLoop = findLoop(s->loopStack.back().loop->subloops(), onEdge->to());
    }
    if (enteringLoop) {
        s->loopStack.push_back({enteringLoop,
                                {},
                                s->colorStack.top()});
        assert(findLoop(enteringLoop->subloops(), onEdge->to()) == nullptr &&
               "Should not enter more than one loop level");
    }
}

void PruningSearcher::testH2HAndUpdate(State *s, Edge *onEdge) {
    if (!s->loopStack.empty() &&
        onEdge->to() == s->loopStack.back().loop->entryNode() &&
        !s->loopStack.back().loopPathFromEntry.empty()  // exclude the case of just entering the loop
            ) {

        Loop *&topLoop = s->loopStack.back().loop;
        Path &topPath = s->loopStack.back().loopPathFromEntry;

        // Update H2H segment coverage

        // Assert using value comparison on Path
        assert(topLoop->h2hSegments().find(topPath) != topLoop->h2hSegments().end() &&
               "Unmatched recorded h2h segment");

        // Update global coverage
        if (uncoveredLoops.find(topLoop) != uncoveredLoops.end()) {
            LoopInfo &info = uncoveredLoops[topLoop];
            auto it = info.uncoveredH2HSegments.find(topPath);  // need to use additional information inside
            if (it != info.uncoveredH2HSegments.end()) {
#if PRUNING_SEARCHER_ECHO_SEGMENT_COVERAGE
                timedInfo() << "PruningSearcher: " << it->tag << " in " << topLoop->name() << " covered by S"
                            << s->getUID() << ", " << info.uncoveredH2HSegments.size() - 1 << " left: ";
                for (auto it2 = info.uncoveredH2HSegments.begin(); it2 != info.uncoveredH2HSegments.end(); ++it2) {
                    if (it2 != it) {
                        progInfo() << it2->tag << " ";  // no timestamp
                    }
                }
                progInfo() << "\n";
//                topPath.dump();
#endif
                info.uncoveredH2HSegments.erase(it);
                s->coveredNewSegment = true;
            }
            if (info.uncoveredH2HSegments.empty() && info.uncoveredH2XSegments.empty()) {
                // The loop is fully covered
                uncoveredLoops.erase(topLoop);
#if PRUNING_SEARCHER_ECHO_SEGMENT_COVERAGE
                timedInfo() << "PruningSearcher: " << topLoop->name() << " is fully covered" << "\n";
#endif
            }
        }

        // Reset loopPathFromEntry
        topPath.clear();
    }
}

bool PruningSearcher::onPrefixOfUncoveredSegment(State *s) {
    for (const auto &item : s->loopStack) {
        Loop *loop = item.loop;
        const Path &path = item.loopPathFromEntry;
        if (uncoveredLoops.find(loop) != uncoveredLoops.end()) {
            for (const auto &target : uncoveredLoops[loop].uncoveredH2HSegments) {
                if (isPrefix(path, target)) {
                    return true;
                }

            }
            for (const auto &target : uncoveredLoops[loop].uncoveredH2XSegments) {
                if (isPrefix(path, target)) {
                    return true;
                }
            }
        }
    }
    return false;
}

Loop *PruningSearcher::findLoop(const set<Loop *> &loops, const Node *entryNode) {
    for (const auto &loop : loops) {
        if (loop->entryNode() == entryNode) return loop;
    }
    return nullptr;
}

bool PruningSearcher::isPrefix(const Path &path, const Path &target, const Edge *additional) {
    if (additional) {
        if (path.size() + 1 > target.size()) return false;
    } else {
        if (path.size() > target.size()) return false;
    }
    unsigned i;
    for (i = 0; i < path.size(); i++) {
        if (path[i] != target[i]) return false;
    }
    if (additional) {
        return additional == target[i];
    } else {
        return true;
    }
}

StateVector PruningSearcher::getNormalStates() const {
    StateVector ret = internalSearcher->getNormalStates();
    if (fetchLevel >= 1) {
        for (const auto &s : postponedStates) {
            ret.push_back(s);
        }
    }
    return ret;
}

State *PruningSearcher::fetch() {
    State *ret = fetchOneState();
    return ret;
}

State *PruningSearcher::fetchOneState() {
    State *ret = internalSearcher->fetch();
    if (ret) return ret;

    if (postponedStates.empty()) return nullptr;
    if (uncoveredLoops.empty()) return nullptr;
    if (fetchLevel >= 1) {
        if (!startedFetchingPostponedStates) {
            timedInfo() << "PruningSearcher: start fetching postponed states\n";
            startedFetchingPostponedStates = true;
        }
        size_t id = std::rand() % postponedStates.size();
        std::swap(postponedStates[id], postponedStates.back());
        ret = postponedStates.back();
        postponedStates.pop_back();
        statReactivateCount++;
#if PRUNING_SEARCHER_ECHO_REACTIVATION
        if (!ret->latestNonOSInst[0].isNull()) {
            timedInfo() << "Reactivate one state at " << ret->latestNonOSInst[0]->sourceContext() << "\n";
        }
#endif
        ret->avoidLoopReductionPostpone = true;
        return ret;
    } else {
        return nullptr;
    }
}

PruningSearcher::~PruningSearcher() {
    if (!uncoveredLoops.empty()) {
        progInfo() << "PruningSearcher: There are loops that left uncovered:\n";
        for (const auto &it : uncoveredLoops) {
            progInfo() << it.first->name() << "\n";
            if (!it.second.uncoveredH2HSegments.empty()) {
                progInfo() << "    H2H: ";
                for (const auto &seg : it.second.uncoveredH2HSegments) {
                    progInfo() << seg.tag << " ";
                }
                progInfo() << "\n";
            }
            if (!it.second.uncoveredH2XSegments.empty()) {
                progInfo() << "    H2X: ";
                for (const auto &seg : it.second.uncoveredH2XSegments) {
                    progInfo() << seg.tag << " ";
                }
                progInfo() << "\n";
            }
        }
    }
    if (!postponedStates.empty()) {
        progInfo() << "PruningSearcher: " << postponedStates.size() << " state(s) left postponed.\n";
    }
    progInfo() << "PruningSearcher: "
               << statPostponeCount << " postpones, "
               << statReactivateCount << " reactivations\n";
}

}