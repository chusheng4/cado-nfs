#ifndef LAS_TODO_H_
#define LAS_TODO_H_

#include <gmp.h>
#include <stack>
#include "las-base.hpp"

struct las_todo_entry {
    mpz_t p;
    /* even for a rational side, the field below is used, since
     * it is needed for the initialization of the q-lattice. All callers
     * of las_todo_push must therefore make sure that a proper argument
     * is provided.
     */
    mpz_t r;
    int side;

    /* some fields which are specific to the descent */
    int depth;
    int iteration;      /* number of times we failed on this prime */

    /* FIXME: Default constructor should be removed! Currently in just to make
       sieve_info_s without a constructor work */
    las_todo_entry() : side(0), depth(0), iteration(0) {
      alloc();
    }
    /* Empty p, r is used for "closing brace" */
    las_todo_entry(const int _side, const int _depth) : side(_side), depth(_depth), iteration(0) {
      alloc();
    }
    las_todo_entry(const mpz_t _p, const mpz_t _r, const int _side, const int _depth = 0, const int _iteration = 0) {
      alloc();
      set(_p, _r, _side, _depth, _iteration);
    }
    las_todo_entry(const las_todo_entry &other) {
      /* Delegating constructors is C++11 :( */
      alloc();
      set(other.p, other.r, other.side, other.depth, other.iteration);
    }
    ~las_todo_entry() {
      mpz_clear(p);
      mpz_clear(r);
    }
    las_todo_entry& operator=(const las_todo_entry &other) {
      if (this != &other)
        set(other.p, other.r, other.side, other.depth, other.iteration);
      return *this;
    }
private:
  void alloc() {
      mpz_init(p);
      mpz_init(r);
  }
  void set(const mpz_t _p, const mpz_t _r, const int _side, const int _depth, const int _iteration) {
      mpz_set(p, _p);
      mpz_set(r, _r);
      side = _side;
      depth = _depth;
      iteration = _iteration;
  }
};

typedef std::stack<las_todo_entry *> las_todo_stack;

#endif
