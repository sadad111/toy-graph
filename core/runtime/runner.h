#ifndef CORE_RUNTIME_RUNNER_H
#define CORE_RUNTIME_RUNNER_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/framework/async_reader.h"
#include "core/framework/graph.h"
#include "core/framework/graph_builder.h"
#include "core/framework/reader.h"
#include "core/framework/writer.h"
#include "core/lib/bucket.h"
#include "core/lib/thread_pool.h"
#include "core/platform/types.h"
#include "core/util/logging.h"

namespace runtime {

class RunnerInterface {
public:
  virtual void run(int iteration) = 0;
  virtual ~RunnerInterface();
  RunnerInterface(){};
  RunnerInterface(const RunnerInterface& r) = delete;
  RunnerInterface& operator=(const RunnerInterface& r) = delete;
protected:
  framework::GraphInterface* graph;
};


template <class MessageT, class EdgeT, class VertexT>
class SimpleRunner : public RunnerInterface {
public:
  void run(int iteration);
  SimpleRunner(platform::int64 vertexNum, platform::int64 edgeNum,
      std::string inputPath, std::string outputPath, int oneVertexSize,
      int oneEdgeSize, int bucketSize = 10000);
  ~SimpleRunner();
  void computeThread(lib::Bucket<EdgeT>* edges);
private:
  std::string inputFile;
  std::string outputFile;
  int bucketSize; // for reader
  framework::ReaderInterface<EdgeT>* reader;
  framework::WriterInterface* writer;
};

RunnerInterface::~RunnerInterface() {
  delete(graph);
}

template <class MessageT, class EdgeT, class VertexT>
SimpleRunner<MessageT, EdgeT, VertexT>::~SimpleRunner() {
  delete(reader);
  delete(writer);
}

template <class MessageT, class EdgeT, class VertexT>
void SimpleRunner<MessageT, EdgeT, VertexT>::computeThread(
    lib::Bucket<EdgeT>* edges) {
  framework::MessageInterface* msg = new MessageT();
  for (int i = 0; i < edges->size(); i++) {
    (*edges)[i].scatter(graph, msg);
  }
  delete(msg);
  //LOG(util::DEBUG) << "delete " << edges;
  delete(edges);
}

template <class MessageT, class EdgeT, class VertexT>
SimpleRunner<MessageT, EdgeT, VertexT>::SimpleRunner(platform::int64 vertexNum,
    platform::int64 edgeNum, std::string inputPath, std::string outputPath,
    int oneVertexSize, int oneEdgeSize, int bs) {
  inputFile = inputPath;
  outputFile = outputPath;
  bucketSize = bs;
  std::unique_ptr<framework::GraphBuilderInterface> gbuild(new
      framework::SimpleGraphBuilder<VertexT>());
  LOG(util::INFO)<<"start build graph, vertex num is "<<vertexNum<<
      ", edge num is "<<edgeNum <<", one vertex size is " << oneVertexSize
      << ", one edge size is " << oneEdgeSize;
  graph = gbuild->build(vertexNum, edgeNum, oneVertexSize, true);
  reader = new framework::AsyncReader<EdgeT>(inputFile, oneEdgeSize);
  writer = new framework::SimpleWriter(outputFile);
};

template <class MessageT, class EdgeT, class VertexT>
void SimpleRunner<MessageT, EdgeT, VertexT>::run(int iteration) {
  LOG(util::INFO)<<"start init every vertex.";
  graph->initAllVertex();
  LOG(util::INFO)<<"finish init every vertex, start run computation.";
  for (int i = 1; i <= iteration; i++) {
    LOG(util::INFO) << "reset reader to read data from beginning again.";
    reader->reset();
    LOG(util::DEBUG) << "reader finished reset.";
    reader->start();
    LOG(util::DEBUG) << "reader starts!";
    LOG(util::INFO) << "try to start thread pool.";
    lib::ThreadPool* pool = new lib::ThreadPool(20);
    pool->start();
    LOG(util::INFO) << "thread pool started.";
    LOG(util::INFO)<<"start iteration: "<<i;
    platform::int64 edgeNumDEBUG = 0, percentCount = 0;
    platform::int64 percent = graph->getEdgeNum() / 100;
    lib::Bucket<EdgeT>* edges = nullptr;
    while (reader->readInToEdge(edges, bucketSize)) {
      platform::int64 edgeNum = edges->size();
      //LOG(util::DEBUG) << "edgeNum is " << edgeNum;
      pool->enQueue(
          std::bind(&SimpleRunner<MessageT, EdgeT, VertexT>::computeThread,
                    this, edges));
      edgeNumDEBUG += edgeNum;
      percentCount += edgeNum;
      if (percentCount >= percent) {
          percentCount = 0;
          LOG(util::INFO) << "iteration " << i
                          << " processed %"
                          << edgeNumDEBUG * 100.0 / graph->getEdgeNum()
                          << " edges";
      }
      /*
      if (edgeNumDEBUG > 100000) {
        reader->stop();
        break;
      }
      */
    }
    pool->stop();
    delete(pool);
    LOG(util::INFO)<<"finished edge process, " << edgeNumDEBUG <<
        " edges processed.";
    LOG(util::INFO)<<"start updating each vertex";
    graph->updateAllVertex();
    LOG(util::INFO)<<"finish iteration: "<<i;
  }
  LOG(util::INFO)<<"finished computation, start saving to file.";
  for (platform::int64 i = 0; i < graph->getVertexNum(); i++) {
    writer->write(graph->getVertexOutput(i));
  }
  LOG(util::INFO)<<"result saved, finished.";
};

}
#endif
