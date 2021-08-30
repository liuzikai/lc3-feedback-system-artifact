//
// Created by liuzikai on 10/9/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/FlowAnalysis/FlowGraphVisualizer.h"
#include "klc3/FlowAnalysis/LoopAnalyzer.h"
#include "klc3/FlowAnalysis/CoverageTracker.h"

#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>
#include <fstream>

namespace klc3 {

llvm::cl::opt<bool> GenerateDotForImages(
        "generate-dot-for-images",
        llvm::cl::desc("Generate dot files instead of png files (default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3DebugCat));

constexpr int FlowGraphVisualizer::EDGE_WEIGHT_MAPPING_COUNT;
constexpr int FlowGraphVisualizer::EDGE_WEIGHT_MAPPING[EDGE_WEIGHT_MAPPING_COUNT];

llvm::cl::OptionCategory KLC3VisualizationCat(
        "KLC3 visualization options",
        "These options impact klc3 flow visualization");

llvm::cl::opt<bool> ShowLineNumber(
        "show-line-number",
        llvm::cl::desc("Show line number in the flow graph (default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3VisualizationCat));

void FlowGraphVisualizer::mergeVisualNodes(VisualNode *dest, VisualNode *src) {
    if (dest == src) return;
    assert(dest->blockColors == src->blockColors && "Unmatched blockColors during merge");
    assert(dest->contentColor == src->contentColor && "Unmatched contentColor during merge");
    assert(dest->visible == src->visible && "Unmatched visibility during merge");
    assert(src->label.empty() && "Discarding label during merge");
    assert(!dest->notMergeAfterward && "Merge dest requires not to be merged afterward");
    assert(!src->notMergeAhead && "Merge src requires not to be merged ahead");
    assert(nodeParent[src] == nodeParent[dest] && "Unmatched node parents");
    assert(dest->outEdges.size() == 1 && dest->outEdges[0]->to == src &&
           "Merge dest should have unique outEdge to src");
    assert(src->inEdges.size() == 1 && src->inEdges[0]->from == dest &&
           "Merge src should have unique inEdge from dest");

    VisualEdge *discardedEdge = src->inEdges[0];

    dest->outEdges = src->outEdges;  // replace outEdges
    for (const auto &edge : dest->outEdges) {
        edge->from = dest;  // change starting point of new outEdge
    }
    dest->contentLines.insert(dest->contentLines.end(), src->contentLines.begin(), src->contentLines.end());
    if (src->notMergeAfterward) dest->notMergeAfterward = true;

    discardedEdge->valid = false;  // the pointer may still left in vEdges or edgeToVEdge
    // Do not do vEdges.erase

    src->valid = false;  // the pointer may still valid in vNodes, nodeToVNode or in a cluster
    // Do not do vNodes.erase since the compression process is in vNodes iteration, it will be done outside

    // Nodes and edges to be discarded will be recycled at compress()
}

bool FlowGraphVisualizer::shouldStopContinuousBlock(VisualNode *node) const {
    assert(node->outEdges.size() == 1 && "Not a continuous block");
    if (node->notMergeAfterward) return true;                     // current node requires not to merge
    VisualNode *next = node->outEdges[0]->to;
    if (next->inEdges.size() >= 2) return true;                   // next node has multiple entries
    if (!next->label.empty()) return true;                        // next node has label
    if (next->blockColors != node->blockColors) return true;      // next node has different color
    if (next->contentColor != node->contentColor) return true;    // next node has different color
    if (next->notMergeAhead) return true;                         // next node requires not to merge
    if (node->outEdges[0]->notMerge) return true;                 // unique outEdge requires not to merge
    if (node->visible != next->visible) return true;              // different visibility
    if (nodeParent.find(node)->second != nodeParent.find(next)->second) return true;  // different layer
    return false;
}

void FlowGraphVisualizer::compressBlock(VisualNode *start) {

    VisualNode *cur = start;

    while (true) {

        visitedNodes.insert(cur);

        if (cur->outEdges.empty()) {
            break;  // end of current block
        } else if (cur->outEdges.size() >= 2) {
            // Start new blocks for all downstream nodes
            for (auto &item: cur->outEdges) {
                if (!item->valid) continue;
                if (visitedNodes.find(item->to) == visitedNodes.end()) {
                    if (item->to != cur) compressBlock(item->to);
                }
            }
            break;  // end of current block
        } else {
            if (shouldStopContinuousBlock(cur)) {
                VisualNode *toNode = cur->outEdges[0]->to;
                if (visitedNodes.find(toNode) == visitedNodes.end()) {
                    if (toNode != cur) compressBlock(toNode);  // start a new block
                }
                break;  // end of current block
            } else {
                mergeVisualNodes(cur, cur->outEdges[0]->to);
                // cur->outEdges got updated, continue
            }
        }
    }
}

void FlowGraphVisualizer::compress() {
    assert(!compressed && "Should not compress for twice");

    visitedNodes.clear();
    for (auto &node : vNodes) {
        if (!node->valid) continue;
        if (visitedNodes.find(node) == visitedNodes.end()) {
            compressBlock(node);
        }
    }

    // Delete invalidated edges and nodes
    for (auto it = nodeToVNode.cbegin(); it != nodeToVNode.cend(); /* no increment */) {
        if (!it->second->valid) it = nodeToVNode.erase(it);
        else ++it;
    }
    for (auto it = edgeToVEdge.cbegin(); it != edgeToVEdge.cend(); /* no increment */) {
        if (!it->second->valid) it = edgeToVEdge.erase(it);
        else ++it;
    }
    for (auto it = vNodes.cbegin(); it != vNodes.cend(); /* no increment */) {
        if (!(*it)->valid) it = vNodes.erase(it);
        else ++it;
    }
    for (auto it = vEdges.cbegin(); it != vEdges.cend(); /* no increment */) {
        if (!(*it)->valid) it = vEdges.erase(it);
        else ++it;
    }

    compressed = true;
}

FlowGraphVisualizer::~FlowGraphVisualizer() {
    for (auto node: vNodes) delete node;
    for (auto edge: vEdges) delete edge;
}

FlowGraphVisualizer::FlowGraphVisualizer(const FlowGraph *fg) : fg(fg) {

    // First pass: build all nodes
    for (const auto &node : fg->allNodes()) {
        auto *vNode = new VisualNode;
        if (node->possibleRETPoint) {
            vNode->notMergeAhead = true;
        }
        if (!node->inst().isNull() && isSpecialInst(node->inst()->instID())) {
            vNode->notMergeAfterward = true;
        }
        vNode->order = node->addr() * 2;  // use address * 2 as order, so that we can put halt node in between
        vNode->label = join("/", node->inst()->labels);
        if (ShowLineNumber) {
            vNode->contentLines.emplace_back(
                    rightPadding(std::to_string(node->inst()->sourceLine), 3, ' ') + " " + node->inst()->sourceContent
            );
        } else {
            vNode->contentLines.emplace_back(
                    node->inst()->sourceContent
            );
        }
        if (node->subroutineEntry) {
            // It's the entry point of a subroutine, do not force to position
            vNode->detachedPosition = true;
        }
        vNodes.emplace(vNode);
        nodeToVNode[node] = vNode;
        nodeParent[vNode] = &topLayer;
    }

    // Second pass: build all edges and halt node
    for (const auto &edge : fg->allEdges()) {
        auto *vEdge = new VisualEdge;
        if (edge->from() == nullptr) {

            // Create a invisible entry node
            auto *entryNode = new VisualNode;
            entryNode->order = nodeToVNode[edge->to()]->order - 1;
            // normal node use addr * 2 as order, leaving space for entry node
            entryNode->label = "(entry)";
            entryNode->visible = false;
            entryNode->notMergeAfterward = true;
            vNodes.emplace(entryNode);

            vEdge->from = entryNode;
        } else {
            vEdge->from = nodeToVNode[edge->from()];
        }
        vEdge->from->outEdges.emplace_back(vEdge);
        if (edge->to() == nullptr) {
            // Create a virtual halt node
            auto *haltNode = new VisualNode;
            haltNode->order = vEdge->from->order + 1;  // normal node use addr * 2 as order, leaving space for halt node
            haltNode->label = "(halt)";
            haltNode->simpleStyle = "dashed";
            haltNode->notMergeAhead = true;
            vNodes.emplace(haltNode);

            vEdge->to = haltNode;
        } else {
            vEdge->to = nodeToVNode[edge->to()];
        }
        vEdge->to->inEdges.emplace_back(vEdge);
        vEdge->attrs.emplace("constraint", "false");  // these edges don't enforce node position

        if (edge->type() == Edge::JSR_EDGE || edge->type() == Edge::RET_EDGE || edge->type() == Edge::JSRR_EDGE ||
            edge->type() == Edge::SUBROUTINE_VIRTUAL_EDGE ||
            edge->type() == Edge::LOOP_H2H_EDGE || edge->type() == Edge::LOOP_H2X_EDGE) {
            vEdge->notMerge = true;
        }
        if (edge->type() == Edge::LOOP_H2H_EDGE || edge->type() == Edge::LOOP_H2X_EDGE) {
            vEdge->visible = false;
            vEdge->style = "dotted";
        } else if (edge->type() == Edge::SUBROUTINE_VIRTUAL_EDGE) {
            vEdge->visible = false;
            vEdge->style = "dashed";
        } else if (edge->type() == Edge::INIT_PC_ENTRY_EDGE) {
            vEdge->style = "dashed";
        }
        vEdges.emplace(vEdge);
        edgeToVEdge[edge] = vEdge;
    }
}

bool FlowGraphVisualizer::isSpecialInst(InstValue::InstID inst) {
    switch (inst) {
        case InstValue::InstID::BR :
        case InstValue::InstID::JMP :
        case InstValue::InstID::JSR :
        case InstValue::InstID::JSRR :
            return true;
        default:
            return false;
    }
}

void FlowGraphVisualizer::colorBasedOnSubroutines() {
    assert(!compressed && "Should not work on compressed graph");

    // Get a collection of all "colors"
    unordered_set<uint16_t> allRawColors;
    for (const auto &node : fg->allNodes()) {
        allRawColors.insert(node->subroutineColors.begin(), node->subroutineColors.end());
    }
    ColorAllocator<uint16_t> colorAllocator(allRawColors.size());

    for (const auto &node : fg->allNodes()) {
        VisualNode *vNode = nodeToVNode[node];
        if (allRawColors.find(node->addr()) != allRawColors.end()) {
            vNode->labelColor = colorAllocator.matchColor(node->addr());
        }
        for (const auto &rawColor : node->subroutineColors) {
            vNode->blockColors.emplace(colorAllocator.matchColor(rawColor));
        }
    }
}

void FlowGraphVisualizer::drawGlobalCoverage(const CoverageTracker *ct, uint16_t initPC) {
    assert(!compressed && "Should not work on compressed graph");

    for (const auto &node : fg->allNodes()) {
        bool nodeCovered = false;
        for (const auto &inEdge : node->runtimeInEdges()) {
            if (!ct->edgeNotCovered(inEdge)) {
                nodeCovered = true;
                break;
            }
        }
        auto *vNode = nodeToVNode[node];
        if (!nodeCovered) vNode->contentColor = "grey";
    }

    for (const auto &edge : fg->allEdges()) {
        VisualEdge *vEdge = edgeToVEdge[edge];
        if (edge->isImproperRET) {
            vEdge->colors.emplace_back("red");
            vEdge->attrs.emplace("arrowhead", "tee");
            vEdge->attrs.emplace("weight", "3");
            vEdge->notMerge = true;
        } else if (ct->edgeNotCovered(edge)) {
            vEdge->attrs.emplace("color", "grey");
        }
    }
}

void FlowGraphVisualizer::generate(const string &outputBaseName) {

    dotNodeCount = 0;
    dotClusterCount = 0;
    completedLayers.clear();
    for (auto &node: vNodes) node->postOrderGenerated = false;
    for (auto &edge: vEdges) edge->generated = false;

    std::stringstream g;

    g << "digraph \"" << "klc3" << "\" {\n";
    generateLayer(g, &topLayer);
    g << "}";

#if 0
    llvm::outs() << g.str();
#endif

    g << "\n";

    generateGraphvizImage(g.str(), outputBaseName);
}

void FlowGraphVisualizer::generateLayer(std::stringstream &g, VisualLayer *layer) {

    // Go into sublayers first
    for (auto sublayer : layer->subLayers()) {
        sublayer->visualizeName = "cluster_" + std::to_string(dotClusterCount++);
        // Write subgraph bracket outside to handle top-level graph in genreate()
        g << "subgraph " << sublayer->visualizeName << " { // " << sublayer->name << "\n";
        generateLayer(g, sublayer);
        g << "}\n";
    }

    // Nodes
    dotWriteEntry(g, "node", {
            {"forcelabels", "true"},
            {"shape",       "none"}
    });
    for (auto &node : vNodes) {
        if (!node->visible) continue;
        if (nodeParent[node] != layer) continue;

        // Allocate name
        node->visualizeName = "N" + std::to_string(dotNodeCount++);

        if (node->simpleStyle.empty()) {

            stringstream ss;
            ss << R"(<<TABLE BORDER="0" CELLBORDER="0">)";
            {
                ss << "<TR>";
                {
                    ss << "<TD VALIGN=\"TOP\">";
                    if (!node->labelColor.empty()) ss << "<FONT COLOR=\"" << node->labelColor << "\">";
                    if (node->labelBold) ss << "<B>";
                    if (node->labelUnderline) ss << "<U>";
                    ss << node->label << " ";
                    if (node->labelUnderline) ss << "</U>";
                    if (node->labelBold) ss << "</B>";
                    if (!node->labelColor.empty()) ss << "</FONT>";
                    ss << "</TD>";

                    for (const auto &color : node->blockColors) {
                        ss << "<TD BGCOLOR=\"" << color << "\"> </TD>";
                    }

                    ss << R"(<TD ALIGN="LEFT" BALIGN="LEFT">)";
                    if (!node->contentColor.empty()) ss << "<FONT COLOR=\"" << node->contentColor << "\">";
                    ss << join("<BR/>", node->contentLines);
                    if (!node->contentColor.empty()) ss << "</FONT>";
                    ss << "</TD>";
                }
                ss << "</TR>";
            }
            ss << "</TABLE>>";

            dotWriteEntry(g,
                          node->visualizeName,
                          {{"label", ss.str()}});

        } else {

            dotWriteEntry(g,
                          node->visualizeName,
                          {{"label", "\"" + node->label + "\""},
                           {"style", node->simpleStyle}});

        }
    }

    // Add hidden edge to maintain order of code
    for (auto it = vNodes.begin(); it != vNodes.end(); ++it) {
        auto it2 = std::next(it);
        if (it2 == vNodes.end()) continue;
        VisualNode *u = *it, *v = *it2;
        if (u->postOrderGenerated) continue;  // already generated
        if (!u->visible || !v->visible) continue;
        if (v->detachedPosition) continue;

        // Draw edge at the inner-most common parent layer of two nodes, when both layers are currentLayer or completed
        VisualLayer *uLayer = nodeParent[u], *vLayer = nodeParent[v];
        if ((uLayer == layer && vLayer == layer) ||
            (completedLayers.find(uLayer) != completedLayers.end() &&
             completedLayers.find(vLayer) != completedLayers.end())) {

            dotWriteEntry(g, u->visualizeName + "->" + v->visualizeName, {{"style", "invis"}});
            u->postOrderGenerated = true;
        }
    }

    // Edges
    for (auto &edge : vEdges) {
        if (edge->generated) continue;  // already generated
        if (!edge->visible) continue;
        if (!edge->from->visible) continue;
        if (!edge->to->visible) continue;
        // Draw edge at the inner-most common parent layer of two nodes, when both layers are currentLayer or completed
        VisualLayer *fromLayer = nodeParent[edge->from], *toLayer = nodeParent[edge->to];
        if ((fromLayer == layer && toLayer == layer) ||
            (completedLayers.find(fromLayer) != completedLayers.end() &&
             completedLayers.find(toLayer) != completedLayers.end())) {

            auto attrs = edge->attrs;
            if (!edge->colors.empty()) {
                attrs["color"] = "\"" + join(":invis:", edge->colors) + "\"";
            }
            if (edge->width != 1) {
                attrs["penwidth"] = std::to_string(edge->width);
            }
            if (!edge->style.empty()) {
                attrs["style"] = edge->style;
            }

            dotWriteEntry(g, edge->from->visualizeName + "->" + edge->to->visualizeName, attrs);
            edge->generated = true;
        }
    }

    completedLayers.emplace(layer);
}

void FlowGraphVisualizer::dotWriteEntry(std::stringstream &g, const string &name,
                                        const unordered_map<string, string> &attrs) {
    g << name << '[';
    if (!attrs.empty()) {
        for (auto it = attrs.begin(); it != attrs.end(); ++it) {
            if (it != attrs.begin()) g << ",";
            // NOTE: do not wrap "" here since this function may used in HTML label
            g << it->first << '=' << it->second;
        }
    }
    g << "]\n";
}

void FlowGraphVisualizer::generateGraphvizImage(const string &dotContent, const string &outputBaseName) {
    if (GenerateDotForImages) {
        string dotFilename = outputBaseName + ".dot";
        FILE *fdot = fopen(dotFilename.c_str(), "w");
        fputs(dotContent.c_str(), fdot);
        fclose(fdot);
    } else {
        string imageFilename = outputBaseName + ".png";
        GVC_t *gvc = gvContext();
        FILE *fimg = fopen(imageFilename.c_str(), "w");
        Agraph_t *g = agmemread(dotContent.c_str());
        gvLayout(gvc, g, "dot");
        gvRender(gvc, g, "png", fimg);
        gvFreeLayout(gvc, g);
        agclose(g);
        fclose(fimg);
        gvFreeContext(gvc);
    }
}

int FlowGraphVisualizer::getEdgePenWidth(int count) {
    for (int i = 0; i < EDGE_WEIGHT_MAPPING_COUNT; i++) {
        if (count <= EDGE_WEIGHT_MAPPING[i]) return i + 1;
    }
    return EDGE_WEIGHT_MAPPING_COUNT + 1;
}

void FlowGraphVisualizer::showOnlySubgraph(const Subgraph &sg) {
    assert(!compressed && "Should not work on compressed graph");  // all vNode must be valid if not compressed
    for (auto &vNode: vNodes) {
        vNode->visible = false;
    }
    for (const auto &node: sg.nodes) {
        assert(nodeToVNode.find(node) != nodeToVNode.end() && "Unknown node");
        nodeToVNode[node]->visible = true;
    }
}

void FlowGraphVisualizer::visualizeLoop(const PathString &outputPath, const string &filename,
                                        const FlowGraph *flowGraph, const Loop *loop, const string &loopColor) {
    auto v2 = std::make_unique<FlowGraphVisualizer>(flowGraph);
    for (const auto &node : loop->nodes()) {
        v2->nodeAppendBlockColor(node, loopColor);
    }
    v2->nodeSetLabelColor(loop->entryNode(), loopColor);
    v2->nodeSetLabelBold(loop->entryNode(), true);
    v2->nodeSetLabelUnderline(loop->entryNode(), true);
    progInfo() << "Visualization of loop " << loop->name() << ":\n";
    ColorAllocator<string> loopSegmentColorAllocator(loop->h2hSegments().size() + loop->h2xSegments().size());
    for (const auto &seg : loop->h2hSegments()) {
        auto color = loopSegmentColorAllocator.matchColor(seg.tag);
        progInfo() << "    H2H segment " << seg.tag << " to color " << color << "\n";
        for (const auto &edge : seg.reconstructedFullPath()) {
            v2->edgeAppendColor(edge, color);
            v2->edgeSetVisible(edge, true);
            v2->edgeSetWidth(edge, 2);
        }
    }
    for (const auto &seg : loop->h2xSegments()) {
        auto color = loopSegmentColorAllocator.matchColor(seg.tag);
        progInfo() << "    H2X segment " << seg.tag << " to color " << color << "\n";
        for (const auto &edge : seg.reconstructedFullPath()) {
            v2->edgeAppendColor(edge, color);
            v2->edgeSetVisible(edge, true);
            v2->edgeSetWidth(edge, 2);
        }
    }
    v2->compress();
    PathString outputFilename(outputPath);
    llvm::sys::path::append(outputFilename, filename);
    v2->generate(outputFilename.c_str());
}

void FlowGraphVisualizer::visualizeAllLoops(const PathString &outputPath, const FlowGraph *flowGraph,
                                            const LoopAnalyzer *loopAnalyzer) {

    // Color all loops
    auto v1 = std::make_unique<FlowGraphVisualizer>(flowGraph);

    auto allLoops = loopAnalyzer->getAllLoops();
    FlowGraphVisualizer::ColorAllocator<Loop *> ca1(allLoops.size());

    // Color all nodes
    for (const auto &loop : allLoops) {
        const string &loopColor = ca1.matchColor(loop);
        v1->nodeSetLabelColor(loop->entryNode(), loopColor);
        v1->nodeSetLabelBold(loop->entryNode(), true);
        v1->nodeSetLabelUnderline(loop->entryNode(), true);
        for (const auto &node : loop->nodes()) {
            v1->nodeAppendBlockColor(node, loopColor);
        }
    }

    v1->compress();
    PathString compressedFlowGraphFilename(outputPath);
    llvm::sys::path::append(compressedFlowGraphFilename, "loop-all");
    v1->generate(compressedFlowGraphFilename.c_str());

    v1.reset();

    // Generate image for each loop
    for (const auto &loop : allLoops) {
        visualizeLoop(outputPath, "loop-" + loop->name(), flowGraph, loop, ca1.matchColor(loop));
    }
}

void FlowGraphVisualizer::visualizeCoverage(const PathString &outputPath, const FlowGraph *flowGraph,
                                            const CoverageTracker *coverageTracker) {

    PathString compressedFlowGraphFilename(outputPath);
    llvm::sys::path::append(compressedFlowGraphFilename, "coverage");
    auto visualizer = std::make_unique<FlowGraphVisualizer>(flowGraph);
    visualizer->colorBasedOnSubroutines();
    visualizer->drawGlobalCoverage(coverageTracker, flowGraph->getInitPC());
    visualizer->compress();
    visualizer->generate(compressedFlowGraphFilename.c_str());

}

void FlowGraphVisualizer::visualizeStatePaths(const PathString &outputPath, const string &filename,
                                              const FlowGraph *flowGraph, const StateVector &states) {
    auto v = std::make_unique<FlowGraphVisualizer>(flowGraph);
    FlowGraphVisualizer::ColorAllocator<State *> ca(states.size());
    for (const auto &state : states) {
        if (state->statePath.empty()) continue;
        const string &color = ca.matchColor(state);
        assert(state->statePath.src()->addr() == flowGraph->getInitPC() && "InitPC edge should be the first of statePath");
        for (const auto &e : state->statePath.reconstructedFullPath()) {
            v->edgeAppendColor(e, color);
        }
    }

    PathString outputFilename(outputPath);
    llvm::sys::path::append(outputFilename, filename);
    v->compress();
    v->generate(outputFilename.c_str());
}


}