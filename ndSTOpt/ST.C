#include <iostream>
#include <limits.h>
#include "sequence.h"
#include "gettime.h"
#include "graph.h"
#include "parallel.h"
#include "unionFind.h"
using namespace std;

struct notMax { bool operator() (intT i) {return i < INT_T_MAX;}};

// Assumes root is negative 
// Not making parent array volatile improves
// performance and doesn't affect correctness
inline intT find(intT i, intT * parent) {
  //if (parent[i] < 0) return i;
  intT j = i; //parent[i];     
  if (parent[j] < 0) return j;
  do j = parent[j];
  while (parent[j] >= 0);
  //note: path compression can happen in parallel in the same tree, so
  //only link from smaller to larger to avoid cycles
  intT tmp;
  while((tmp=parent[i])<j){ parent[i]=j; i=tmp;} 
  return j;
}

pair<intT*, intT> st(edgeArray<intT> EA){
  edge<intT>* E = EA.E;
  intT m = EA.nonZeros;
  intT n = EA.numRows;
  intT *parents = newA(intT,n);
  parallel_for (intT i=0; i < n; i++) parents[i] = -1;
  intT *hooks = newA(intT,n);
  parallel_for (intT i=0; i < n; i++) hooks[i] = INT_T_MAX;
  
  parallel_for (intT i = 0; i < m; i++) {
    intT u = E[i].u, v = E[i].v;
    while(1){
      u = find(u,parents);
      v = find(v,parents);
      if(u == v) break;
      if(u > v) swap(u,v);
      //if successful, store the ID of the edge used in hooks[u]
      if(hooks[u] == INT_T_MAX && __sync_bool_compare_and_swap(&hooks[u],INT_T_MAX,i)){
	parents[u]=v;
	break;
      }
    }
  }

  //get the IDs of the edges in the spanning forest
  _seq<intT> stIdx = sequence::filter((intT*) hooks, n, notMax());
  
  free(parents); free(hooks); 
  cout<<"nInSt = "<<stIdx.n<<endl;
  return pair<intT*,intT>(stIdx.A, stIdx.n);
}
