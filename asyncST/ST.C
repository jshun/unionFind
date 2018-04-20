#include <iostream>
#include <limits.h>
#include "sequence.h"
#include "gettime.h"
#include "graph.h"
#include "parallel.h"
using namespace std;

struct notMax { bool operator() (intT i) {return i < INT_T_MAX;}};

inline intT find(intT i, intT* parent) {
  while(i != parent[i])
    i = parent[i];
  return i;
}

inline intT findCompress(intT i, intT* parent) {
  intT j = i; //parent[i];
  if (parent[j] == j) return j;
  do j = parent[j];
  while (parent[j] != j);
  //note: path compression can happen in parallel in the same tree, so
  //only link from smaller to larger to avoid cycles
  intT tmp;
  while((tmp=parent[i])<j){ parent[i]=j; i=tmp;} 
  return j;
}

inline void findCheck(intT i, intT* parent) {
  while(i < parent[i]) {
    i = parent[i];
  }
  if(i > parent[i]) { cout << "error\n"; exit(0); }
}

inline intT findSplit(intT i, intT* parent) {
  while(1) {
    intT v = parent[i];
    intT w = parent[v];
    if(v == w) return v;
    else {
      __sync_bool_compare_and_swap(&parent[i],v,w);
      i = v;
    }
  }
}

inline intT findHalve(intT i, intT* parent) {
  while(1) {
    intT v = parent[i];
    intT w = parent[v];
    if(v == w) return v;
    else {
      __sync_bool_compare_and_swap(&parent[i],v,w);
      //i = w;
      i = parent[i];
    }
  }
}

inline void unite(intT i, edge<intT>* E, intT* parents, intT* hooks) {
  while(1){
    intT u = findCompress(E[i].u,parents);
    intT v = findCompress(E[i].v,parents);
    if(u == v) break;
    else if(u < v && parents[u] == u && __sync_bool_compare_and_swap(&parents[u],u,v)) { hooks[u] = i; return;}
    else if(v < u && parents[v] == v && __sync_bool_compare_and_swap(&parents[v],v,u)) { hooks[v] = i; return;}
  }
}

inline void uniteEarly(intT i, edge<intT>* E, intT* parents, intT* hooks) {
  intT u = E[i].u;
  intT v = E[i].v;
  while(1) {
    if(u == v) return;
    if(v < u) swap(u,v);
    if(__sync_bool_compare_and_swap(&parents[u],u,v)) { hooks[u] = i; return;}
    intT z = parents[u];
    intT w = parents[z];
    __sync_bool_compare_and_swap(&parents[u],z,w);
    u = z;
  }
}

pair<intT*, intT> st(edgeArray<intT> EA){
  edge<intT>* E = EA.E;
  intT m = EA.nonZeros;
  intT n = EA.numRows;
  intT *parents = newA(intT,n);
  parallel_for (intT i=0; i < n; i++) parents[i] = i;
  intT *hooks = newA(intT,n);
  parallel_for (intT i=0; i < n; i++) hooks[i] = INT_T_MAX;
  edge<intT>* st = newA(edge<intT>,m);
  parallel_for (intT i = 0; i < m; i++) {
    unite(i,E,parents,hooks);
  }
  
  _seq<intT> stIdx = sequence::filter((intT*) hooks, n, notMax());
  //parallel_for(long i=0;i<n;i++) findCheck(i,parents);
  
  free(parents); free(hooks); free(st);
  cout<<"nInSt = "<<stIdx.n<<endl;
  return pair<intT*,intT>(stIdx.A, stIdx.n);
}
