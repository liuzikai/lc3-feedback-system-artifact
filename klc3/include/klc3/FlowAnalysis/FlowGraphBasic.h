//
// Created by liuzikai on 11/16/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_FLOWGRAPHBASIC_H
#define KLC3_FLOWGRAPHBASIC_H

#include "klc3/Common.h"

namespace klc3 {

class Node;

class Edge;

class Path {
public:

    Node *src() const;

    Node *dest() const;

    void append(Edge *edge);

    /**
     * Non-runtime edges are allowed, which are recognized as guiding edges (see Edge::isGuidingEdge).
     */

    /**
     * Preserve only the first edge, guiding edges (see Edge::isGuidingEdge), and the last edge. Should only be called
     * after all runtime edges are constructed.
     */
    void compress();

    /**
     * Append an edge with compression. Should only be called after all runtime edges are constructed.
     * @param edge
     * @param isLast
     * @return Whether the edge is actually appended
     */
    bool appendCompressed(Edge *edge, bool isLast = false);

    void pop_back() { es.pop_back(); }

    void dump() const;

    vector<Edge *> reconstructedFullPath() const;

    string tag;

    auto size() const { return es.size(); }
    auto empty() const { return es.empty(); }
    void clear() { es.clear(); }

    bool operator==(const Path &p) const { return es == p.es; }
    bool operator!=(const Path &p) const { return es != p.es; }
    bool operator<(const Path&p) const { return es < p.es; }
    bool operator>(const Path&p) const { return es > p.es; }
    Edge *&operator[](size_t i) { return es[i]; }
    const Edge *operator[](size_t i) const { return es[i]; }

    auto begin() { return es.begin(); }
    auto begin() const { return es.begin(); }
    auto cbegin() const { return es.cbegin(); }
    auto end() { return es.end(); }
    auto end() const { return es.end(); }
    auto cend() const { return es.cend(); }

    auto back() const { return es.back(); }
    auto back() { return es.back(); }

protected:
    vector<Edge *> es;
};

class Subgraph;

}

#endif  // KLC3_FLOWGRAPHBASIC_H
