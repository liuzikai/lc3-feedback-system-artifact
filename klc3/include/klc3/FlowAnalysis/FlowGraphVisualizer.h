//
// Created by liuzikai on 10/9/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_FLOWGRAPHVISUALIZER_H
#define KLC3_FLOWGRAPHVISUALIZER_H

#include "FlowGraph.h"
#include "CoverageTracker.h"
#include "LoopAnalyzer.h"

namespace klc3 {

class Node;
class Edge;

class LoopAnalyzer;
class CoverageTracker;

class FlowGraphVisualizer {
public:

    explicit FlowGraphVisualizer(const FlowGraph *fg);

    FlowGraphVisualizer(const FlowGraphVisualizer &) = delete;

    ~FlowGraphVisualizer();

    void nodeAppendBlockColor(Node *node, const string &color) { nodeToVNode[node]->blockColors.emplace(color); }

    void nodeSetLabelColor(Node *node, const string &color) { nodeToVNode[node]->labelColor = color; }

    void nodeSetContentColor(Node *node, const string &color) { nodeToVNode[node]->contentColor = color; }

    void nodeSetLabelBold(Node *node, bool bold) { nodeToVNode[node]->labelBold = bold; }

    void nodeSetLabelUnderline(Node *node, bool underline) { nodeToVNode[node]->labelUnderline = underline; }

    void nodeSetVisible(Node *node, bool visible) { nodeToVNode[node]->visible = visible; }

    void edgeAppendColor(Edge *edge, const string &color) { edgeToVEdge[edge]->colors.emplace_back(color); }

    int edgeGetWidth(Edge *edge) const{ return edgeToVEdge.find(edge)->second->width; }

    void edgeSetWidth(Edge *edge, int width) { edgeToVEdge[edge]->width = width; }

    void edgeSetVisible(Edge *edge, bool visible) { edgeToVEdge[edge]->visible = visible; }

    void colorBasedOnSubroutines();

    void drawGlobalCoverage(const CoverageTracker *ct, uint16_t initPC);

    void showOnlySubgraph(const Subgraph &sg);

    void compress();

    void generate(const string &outputBaseName);

    template<typename T>
    class ColorAllocator {
    public:
        explicit ColorAllocator(const int count, bool light = false) {
            if (count <= 8) {
                if (light) {
                    scheme = "/pastel18/";
                    maxCount = 8;
                } else {
                    scheme = "/set28/";
                    maxCount = 8;
                }
            } else if (count <= 12) {
                scheme = "/set312/";
                maxCount = 12;
            } else {
                maxCount = count;
            }
        }

        string matchColor(const T &rawValue) {
            auto it = colorMap.find(rawValue);
            if (it == colorMap.end()) {
                assert(colorMap.size() < maxCount && "Too many colors");
                it = colorMap.emplace(rawValue, colorMap.size() + 1).first;
            }
            if (!scheme.empty()) {
                return scheme + std::to_string(it->second);
            } else {
                return std::to_string((float) (it->second - 1) / (float) maxCount) + " 1 1";
            }
        }

    private:
        string scheme;
        size_t maxCount;
        map<T, size_t> colorMap;
    };

    // Output png/dot with given path and filename
    static void visualizeLoop(const PathString &outputPath, const string &filename,
                              const FlowGraph *flowGraph, const Loop *loop, const string &color = "/set28/1");

    // Output loops.png/dot & loop-LOOP_NAME.png/dot for each loop
    static void visualizeAllLoops(const PathString& outputPath,
                                  const FlowGraph *flowGraph, const LoopAnalyzer *loopAnalyzer);

    // Output coverage.png/dot
    static void visualizeCoverage(const PathString& outputPath,
                                  const FlowGraph *flowGraph, const CoverageTracker *coverageTracker);

    // Output png/dot with given path and filename
    static void visualizeStatePaths(const PathString& outputPath,
                                    const string &filename, const FlowGraph *flowGraph, const StateVector& states);

private:

    const FlowGraph *fg;

    class VisualNode;

    class VisualEdge;

    // ================================ Compression ================================

    void compressBlock(VisualNode *cur);

    bool shouldStopContinuousBlock(VisualNode *node) const;

    void mergeVisualNodes(VisualNode *dest, VisualNode *src);

    unordered_set<VisualNode *> visitedNodes;

    bool compressed = false;

    // ================================ Build from Flowgraph ================================

    class VisualNode {
    public:
        vector<VisualEdge *> outEdges;
        vector<VisualEdge *> inEdges;

        bool notMergeAhead = false;
        bool notMergeAfterward = false;
        int order = 0;
        bool detachedPosition = false;

        string label;
        string labelColor;
        bool labelBold = false;
        bool labelUnderline = false;

        string simpleStyle;

        set<string> blockColors;

        vector<string> contentLines;
        string contentColor;

        string visualizeName;  // allocate when generate
        bool visible = true;

        bool postOrderGenerated = false;  // for generation

    private:
        bool valid = true;  // after compression, a node may be no longer valid

        friend void FlowGraphVisualizer::mergeVisualNodes(VisualNode *dest, VisualNode *src);

        friend void FlowGraphVisualizer::compressBlock(VisualNode *cur);

        friend void FlowGraphVisualizer::compress();
    };

    struct VisualNodeLess {
        inline bool operator()(const VisualNode *const lhs, const VisualNode *const rhs) const {
            return (lhs->order < rhs->order);
        }
    };

    class VisualEdge {
    public:

        VisualNode *from = nullptr;
        VisualNode *to = nullptr;

        vector<string> colors;
        int width = 1;
        string style;

        unordered_map<string, string> attrs;
        /*
         * arrowhead:
         * weight:
         */

        bool notMerge = false;
        bool visible = true;

        bool generated = false;  // for generation

    private:
        bool valid = true;  // after compression, a node may be no longer valid

        friend void FlowGraphVisualizer::mergeVisualNodes(VisualNode *dest, VisualNode *src);

        friend void FlowGraphVisualizer::compressBlock(VisualNode *cur);

        friend void FlowGraphVisualizer::compress();
    };

    set<VisualNode *, VisualNodeLess> vNodes;  // holds lifecycle
    unordered_set<VisualEdge *> vEdges;        // holds lifecycle

    unordered_map<const Node *, VisualNode *> nodeToVNode;
    unordered_map<const Edge *, VisualEdge *> edgeToVEdge;

    class VisualLayer {
    public:

        ~VisualLayer() { for (auto &subLayer : subLayers_) delete subLayer; }

        VisualLayer *newSubLayer() {
            auto subLayer = new VisualLayer;
            subLayers_.emplace_back(subLayer);
            return subLayer;
        }

        const vector<VisualLayer *> &subLayers() const { return subLayers_; }

        string name;
        string visualizeName;  // allocate when generate

    private:
        vector<VisualLayer *> subLayers_;
    };

    VisualLayer topLayer;

    unordered_map<VisualNode *, VisualLayer *> nodeParent;

    static bool isSpecialInst(InstValue::InstID inst);

    // ================================ Organization ================================

    // ================================ Visualization ================================

    int dotNodeCount = 0;
    int dotClusterCount = 0;
    unordered_set<VisualLayer *> completedLayers;  // layers that has finished generating all nodes

    void generateLayer(stringstream &g, VisualLayer *layer);

    static void dotWriteEntry(std::stringstream &g, const string &name, const unordered_map<string, string> &attrs);

    static void generateGraphvizImage(const string &dotContent, const string &outputBaseName);

    static constexpr int EDGE_WEIGHT_MAPPING_COUNT = 7;
    static constexpr int EDGE_WEIGHT_MAPPING[EDGE_WEIGHT_MAPPING_COUNT] = {1, 4, 16, 64, 256, 1024, 4096};

    static int getEdgePenWidth(int count);
};



}

#endif //KLC3_FLOWGRAPHVISUALIZER_H
