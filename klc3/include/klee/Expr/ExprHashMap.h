//===-- ExprHashMap.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRHASHMAP_H
#define KLEE_EXPRHASHMAP_H

#include "klee/Expr/Expr.h"

#include <unordered_map>
#include <unordered_set>

namespace klee {

  namespace util {
    struct ExprHash  {
      unsigned operator()(const ref<Expr> &e) const { return e->hash(); }
    };

  // NOTE: [liuzikai] add raw pointer compare
    struct ExprPtrHash  {
      auto operator()(const ref<Expr> &e) const { return std::hash<const Expr *>()(e.get()); }
    };
    
    struct ExprCmp {
      bool operator()(const ref<Expr> &a, const ref<Expr> &b) const {
        return a==b;
      }
    };

    // NOTE: [liuzikai] add raw pointer compare
    struct ExprPrtCmp {
      bool operator()(const ref<Expr> &a, const ref<Expr> &b) const {
        return a.get() == b.get();
      }
    };
  }

  template <class T>
  class ExprHashMap
      : public std::unordered_map<ref<Expr>, T, klee::util::ExprHash,
                                  klee::util::ExprCmp> {};

  // NOTE: [liuzikai] add a map using raw pointer compare
  template <class T>
  class ExprHashRawPtrMap
: public std::unordered_map<ref<Expr>, T, klee::util::ExprPtrHash,
                                  klee::util::ExprPrtCmp> {};

  typedef std::unordered_set<ref<Expr>, klee::util::ExprHash,
                             klee::util::ExprCmp>
      ExprHashSet;
} // namespace klee

#endif /* KLEE_EXPRHASHMAP_H */
