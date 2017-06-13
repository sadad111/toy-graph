#ifndef CORE_FRAMEWORK_GRAPH_H
#define CORE_FRAMEWORK_GRAPH_H

#include <vector>

#include "core/framework/message.h"
#include "core/framework/vertex.h"
#include "core/platform/types.h"

namespace framework {

class GraphInterface {
public:
  GraphInterface(platform::int64 vNum, platform::int64 eNum);
  virtual void updateAllVertex() = 0;
  virtual void updateOneVertex(platform::int64 idx) = 0;
  virtual void initOneVertex(platform::int64 idx) = 0;
  virtual void getVertexInfo(platform::int64 idx, MessageInterface* msg) = 0;
  virtual void scatter(platform::int64 idx, MessageInterface* msg) = 0;
  virtual ~GraphInterface();
  virtual platform::int64 getEdgeNum() = 0;
protected:
  platform::int64 vertexNum, edgeNum, vertexBufSize;
  void* vertexBuf;
};

class SimpleGraph : public GraphInterface {
public:
  SimpleGraph(platform::int64 vNum, platform::int64 eNum);
  SimpleGraph() = delete;
  SimpleGraph(const SimpleGraph& sg) = delete;
  SimpleGraph& operator=(const SimpleGraph& sg) = delete;
  ~SimpleGraph() override;
  void updateOneVertex(platform::int64 idx) override;
  void updateAllVertex() override;
  void initOneVertex(platform::int64 idx) override;
  void getVertexInfo(platform::int64 idx, MessageInterface* msg) override;
  void scatter(platform::int64 idx, MessageInterface* msg) override;
  void setVertex(VertexInterface* v);
  platform::int64 getEdgeNum() override;
private:
  VertexInterface* vertex;
};

}
#endif
