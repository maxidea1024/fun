#include "fun/net/event_loop.h"
#include "fun/net/inspect/process_inspector.h"
#include "memcache_server.h"

#include <stdio.h>
#ifdef HAVE_TCMALLOC
#include <gperftools/heap-profiler.h>
#include <gperftools/malloc_extension.h>
#endif

using namespace fun::net;

int main(int argc, char* argv[]) {
#ifdef HAVE_TCMALLOC
  MallocExtension::Initialize();
#endif
  int items = argc > 1 ? atoi(argv[1]) : 10000;
  int keylen = argc > 2 ? atoi(argv[2]) : 10;
  int valuelen = argc > 3 ? atoi(argv[3]) : 100;
  EventLoop loop;
  MemcacheServer::Options options;
  MemcacheServer server(&loop, options);

  printf(
      "sizeof(Item) = %zd\npid = %d\nitems = %d\nkeylen = %d\nvaluelen = %d\n",
      sizeof(Item), Process::CurrentPid(), items, keylen, valuelen);
  char key[256] = {0};
  String value;
  for (int i = 0; i < items; ++i) {
    snprintf(key, sizeof key, "%0*d", keylen, i);
    value.Assign(valuelen, "0123456789"[i % 10]);
    ItemPtr item(Item::makeItem(key, 0, 0, valuelen + 2, 1));
    item->append(value.data(), value.size());
    item->append("\r\n", 2);
    fun_check(item->EndsWithCRLF());
    bool exists = false;
    bool stored = server.StoreItem(item, Item::kAdd, &exists);
    fun_check(stored);
    (void)stored;
    fun_check(!exists);
  }
  Inspector::ArgList arg;
  printf("==========\n%s\n",
         ProcessInspector::overview(HttpRequest::kGet, arg).c_str());
  // TODO: print bytes per item, overhead percent
  fflush(stdout);
#ifdef HAVE_TCMALLOC
  char buf[8192];
  MallocExtension::instance()->GetStats(buf, sizeof buf);
  printf("%s\n", buf);
  HeapProfilerDump("end");

/*
  // only works for tcmalloc_debug
  int blocks = 0;
  size_t total = 0;
  int histogram[kMallocHistogramSize] = { 0, };
  MallocExtension::instance()->MallocMemoryStats(&blocks, &total, histogram);
  printf("==========\nblocks = %d\ntotal = %zd\n", blocks, total);
  for (int i = 0; i < kMallocHistogramSize; ++i) {
    printf("%d = %d\n", i, histogram[i]);
  }
*/
#endif
}
