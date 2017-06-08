#include "core/framework/reader.h"
#include "core/platform/malloc.cc"

namespace framework {

SimpleReader::SimpleReader(platform::FileInterface* f) {
  file = f;
  offset = readerBufSize;
  bufSize = 0;
  buf = (char*)platform::Malloc(readerBufSize);
}

SimpleReader::~SimpleReader() {
  platform::Free(buf);
  delete(file);
}

bool SimpleReader::get(EdgeInterface& edge) {
  if (offset >= readerBufSize) {
    file->sequentialRead(buf, readerBufSize, bufSize);
    if (bufSize == 0) return false;
    offset = 0;
  }
  return edge.read(buf, bufSize, offset);
}

}
