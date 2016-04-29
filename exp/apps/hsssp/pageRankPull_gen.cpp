/** Residual based Page Rank -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2013, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @section Description
 *
 * Compute pageRank Pull version using residual on distributed Galois.
 *
 * @author Gurbinder Gill <gurbinder533@gmail.com>
 */

#include <iostream>
#include <limits>
#include "Galois/Galois.h"
#include "Lonestar/BoilerPlate.h"
#include "Galois/gstl.h"

#include "Galois/Runtime/CompilerHelperFunctions.h"

#include "Galois/Dist/OfflineGraph.h"
#include "Galois/Dist/hGraph.h"
#include "Galois/DistAccumulator.h"

static const char* const name = "PageRank - Compiler Generated Distributed Heterogeneous";
static const char* const desc = "PageRank Pull version on Distributed Galois.";
static const char* const url = 0;

namespace cll = llvm::cl;
static cll::opt<std::string> inputFile(cll::Positional, cll::desc("<input file>"), cll::Required);
static cll::opt<unsigned int> maxIterations("maxIterations", cll::desc("Maximum iterations"), cll::init(4));
static cll::opt<float> tolerance("tolerance", cll::desc("tolerance"), cll::init(0.01));
static cll::opt<bool> verify("verify", cll::desc("Verify ranks by printing to the output stream"), cll::init(false));


static const float alpha = (1.0 - 0.85);
//static const float  tolerance = 0.1;
struct PR_NodeData {
  float value;
  std::atomic<int> nout;
};

typedef hGraph<PR_NodeData, void> Graph;
typedef typename Graph::GraphNode GNode;

struct InitializeGraph {
  Graph* graph;

  void static go(Graph& _graph) {

     struct Syncer_0 {
    	static int extract(GNode src, const struct PR_NodeData & node){ return node.nout; }
    	static void reduce (GNode src,struct PR_NodeData & node, int y) { Galois::atomicAdd(node.nout, y);}
    	static void reset (GNode src,struct PR_NodeData & node ){node.nout = 0 ; }
    	typedef int ValTy;
    };
     struct SyncerPull_0 {
    	static float extract( GNode src,const struct PR_NodeData & node){ return node.value; }
    	static void setVal (GNode src,struct PR_NodeData & node, float y) {node.value = y; }
    	typedef float ValTy;
    };
     struct SyncerPull_1 {
    	static int extract( GNode src,const struct PR_NodeData & node){ return node.nout; }
    	static void setVal (GNode src,struct PR_NodeData & node, int y) {node.nout = y; }
    	typedef int ValTy;
    };
    Galois::do_all(_graph.begin(), _graph.end(), InitializeGraph{ &_graph }, Galois::loopname("Init"), Galois::write_set("sync_pull", "this->graph", "struct PR_NodeData &", "struct PR_NodeData &", "value" , "float"), Galois::write_set("sync_pull", "this->graph", "struct PR_NodeData &", "struct PR_NodeData &", "nout" , "int"), Galois::write_set("sync_push", "this->graph", "struct PR_NodeData &", "struct PR_NodeData &" , "nout", "int" , "{ Galois::atomicAdd(node.nout, y);}",  "{node.nout = 0 ; }"));
    _graph.sync_push<Syncer_0>();
    
    _graph.sync_pull<SyncerPull_0>();
    
    _graph.sync_pull<SyncerPull_1>();
    
  }

  void operator()(GNode src) const {
    PR_NodeData& sdata = graph->getData(src);
    sdata.value = 1.0 - alpha;
    Galois::atomicAdd(sdata.nout, 0);
    for(auto nbr = graph->edge_begin(src); nbr != graph->edge_end(src); ++nbr){
      GNode dst = graph->getEdgeDst(nbr);
      PR_NodeData& ddata = graph->getData(dst);
      Galois::atomicAdd(ddata.nout, 1);
    }
  }
};



struct InitializeGraph_value {
  Graph* graph;

  void static go(Graph& _graph) {

    struct SyncerPull_0 {
      static float extract( GNode src,const struct PR_NodeData & node){ return node.value; }
      static void setVal (GNode src,struct PR_NodeData & node, float y) {node.value = y; }
      typedef float ValTy;
    };

    Galois::do_all(_graph.begin(), _graph.end(), InitializeGraph_value{ &_graph }, Galois::loopname("Init"));
    _graph.sync_pull<SyncerPull_0>();
  }

  void operator()(GNode src) const {
    PR_NodeData& sdata = graph->getData(src);
    sdata.value = 1.0 - alpha;
  }
};

struct PageRank_pull {
  Graph* graph;

  void static go(Graph& _graph) {

       struct SyncerPull_0 {
      	static float extract( GNode src,const struct PR_NodeData & node){ return node.value; }
      	static void setVal (GNode src,struct PR_NodeData & node, float y) {node.value = y; }
      	typedef float ValTy;
      };

     do{
         DGAccumulator_accum.reset();
         Galois::do_all(_graph.begin(), _graph.end(), PageRank_pull { &_graph }, Galois::loopname("pageRank"), Galois::write_set("sync_pull", "this->graph", "struct PR_NodeData &", "struct PR_NodeData &", "value" , "float"));
      _graph.sync_pull<SyncerPull_0>();
     }while(DGAccumulator_accum.reduce());
      
  }

  static Galois::DGAccumulator<int> DGAccumulator_accum;
  void operator()(GNode src)const {
    PR_NodeData& sdata = graph->getData(src);
    float sum = 0;
    for(auto nbr = graph->edge_begin(src); nbr != graph->edge_end(src); ++nbr){
      GNode dst = graph->getEdgeDst(nbr);
      PR_NodeData& ddata = graph->getData(dst);
      unsigned dnout = ddata.nout;
      if(ddata.nout > 0){
        sum += ddata.value/dnout;
      }
    }

    float pr_value = sum*(1.0 - alpha) + alpha;
    float diff = std::fabs(pr_value - sdata.value);

    if(diff > tolerance){
      sdata.value = pr_value;
      DGAccumulator_accum+= 1;
    }
  }
};
Galois::DGAccumulator<int>  PageRank_pull::DGAccumulator_accum;

int main(int argc, char** argv) {
  try {

    LonestarStart(argc, argv, name, desc, url);
    auto& net = Galois::Runtime::getSystemNetworkInterface();
    Galois::Timer T_total, T_offlineGraph_init, T_hGraph_init, T_init, T_pageRank1, T_pageRank2, T_pageRank3;

    T_total.start();

    T_offlineGraph_init.start();
    OfflineGraph g(inputFile);
    T_offlineGraph_init.stop();
    std::cout << g.size() << " " << g.sizeEdges() << "\n";

    T_hGraph_init.start();
    Graph hg(inputFile, net.ID, net.Num);
    T_hGraph_init.stop();

    std::cout << "InitializeGraph::go called\n";

    T_init.start();
    InitializeGraph::go(hg);
    T_init.stop();
    Galois::Runtime::getHostBarrier().wait();


    // Verify
    if(verify){
      if(net.ID == 0) {
        for(auto ii = hg.begin(); ii != hg.end(); ++ii) {
          std::cout << "[" << *ii << "]  " << hg.getData(*ii).nout << "\n";
        }
      }
    }

    std::cout << "PageRank::go run1 called  on " << net.ID << "\n";
    T_pageRank1.start();
      PageRank_pull::go(hg);
    T_pageRank1.stop();

    std::cout << "[" << net.ID << "]" << " Total Time : " << T_total.get() << " offlineGraph : " << T_offlineGraph_init.get() << " hGraph : " << T_hGraph_init.get() << " Init : " << T_init.get() << " PageRank1 : " << T_pageRank1.get() << " (msec)\n\n";

    Galois::Runtime::getHostBarrier().wait();
    InitializeGraph_value::go(hg);

    std::cout << "PageRank::go run2 called  on " << net.ID << "\n";
    T_pageRank2.start();
      PageRank_pull::go(hg);
    T_pageRank2.stop();

    std::cout << "[" << net.ID << "]" << " Total Time : " << T_total.get() << " offlineGraph : " << T_offlineGraph_init.get() << " hGraph : " << T_hGraph_init.get() << " Init : " << T_init.get() << " PageRank2 : " << T_pageRank2.get() << " (msec)\n\n";

    Galois::Runtime::getHostBarrier().wait();
    InitializeGraph_value::go(hg);

    std::cout << "PageRank::go run3 called  on " << net.ID << "\n";
    T_pageRank3.start();
      PageRank_pull::go(hg);
    T_pageRank3.stop();





    // Verify
    if(verify){
      if(net.ID == 0) {
        for(auto ii = hg.begin(); ii != hg.end(); ++ii) {
          std::cout << "[" << *ii << "]  " << hg.getData(*ii).value << "\n";
        }
      }
    }

    T_total.stop();

    auto mean_time = (T_pageRank1.get() + T_pageRank2.get() + T_pageRank3.get())/3;

    std::cout << "[" << net.ID << "]" << " Total Time : " << T_total.get() << " offlineGraph : " << T_offlineGraph_init.get() << " hGraph : " << T_hGraph_init.get() << " Init : " << T_init.get() << " PageRank1 : " << T_pageRank1.get() << " PageRank2 : " << T_pageRank2.get() << " PageRank3 : " << T_pageRank3.get() <<" PageRank mean time (3 runs ) (" << maxIterations << ") : " << mean_time << "(msec)\n\n";

    return 0;
  } catch (const char* c) {
      std::cerr << "Error: " << c << "\n";
      return 1;
  }
}
