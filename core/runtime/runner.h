#ifndef CORE_RUNTIME_RUNNER_H
#define CORE_RUNTIME_RUNNER_H

#include <iostream>
#include <string>

#include "core/framework/graph.h"
#include "core/framework/reader.h"
#include "core/platform/types.h"

namespace runtime {

class RunnerInterface {
public:
  virtual void run(int iteration) = 0;
  virtual ~RunnerInterface();
protected:
  framework::GraphInterface* graph;
  framework::ReaderInterface* reader;
};


template <class MessageT, class EdgeT, class VertexT>
class SimpleRunner : public RunnerInterface {
public:
  void run(int iteration);
  SimpleRunner(platform::int64 vertexNum, platform::int64 edgeNum,
      std::string filePath);
private:
  std::string file;
};


RunnerInterface::~RunnerInterface() {
  delete(graph);
  delete(reader);
}

template <class MessageT, class EdgeT, class VertexT>
SimpleRunner<MessageT, EdgeT, VertexT>::SimpleRunner(platform::int64 vertexNum,
    platform::int64 edgeNum, std::string filePath) {
  file = filePath;
  framework::GraphBuilderInterface* gbuild = new
      framework::SimpleGraphBuilder<VertexT>();
  std::cout<<"start build graph, vertex num is "<<vertexNum<<std::endl;
  graph = gbuild->build(vertexNum, edgeNum);
  delete(gbuild);
};

template <class MessageT, class EdgeT, class VertexT>
void SimpleRunner<MessageT, EdgeT, VertexT>::run(int iteration) {
  for (int i = 1; i <= iteration; i++) {
    delete(reader);
    std::cout<<"iteration: "<<i<<std::endl;
    reader = new framework::SimpleReader(file);
    framework::EdgeInterface* tmp = new EdgeT();
    framework::MessageInterface* msg = new MessageT();
    while (reader->get(tmp)) {
      tmp->scatter(graph, msg);
    }
    graph->update();
  }
};

}

#endif
