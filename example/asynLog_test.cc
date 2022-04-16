
// #include <mymuduo/Logger.h>
#include <mymuduo/AsyncLogging.h>
#include <mymuduo/Timestamp.h>

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace mymuduo;
off_t kRollSize = 500*1000*1000;

AsyncLogging* g_asyncLog = NULL;

void asyncOutput(const char* msg, int len)
{
  g_asyncLog->append(msg, len);
}

void bench(bool longLog)
{
  // mymuduo::Logger::setOutput(asyncOutput);
  Logger::setOutPut(asyncOutput);

  int cnt = 0;
  const int kBatch = 1000;
  std::string empty = " ";
  // std::string longStr(3000, 'X');
  std::string longStr(300, 'X');
  longStr += " ";

  for (int t = 0; t < 30; ++t)
  {
    Timestamp start = mymuduo::Timestamp::now();
    for (int i = 0; i < kBatch; ++i)
    {
      // LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
      //          << (longLog ? longStr : empty)
      //          << cnt;
      LOG_INFO("Hello 0123456789 abcdefghijklmnopqrstuvwxyz %s%d", longStr, cnt);
      // LOG_INFO("test\n");
      ++cnt;
    }
    Timestamp end = Timestamp::now();
    printf("%f\n", timeDifference(end, start)*1000000/kBatch);
    struct timespec ts = { 0, 500*1000*1000 };
    nanosleep(&ts, NULL);
  }
}

int main(int argc, char* argv[])
{
  {
    // set max virtual memory to 2GB.
    size_t kOneGB = 1000*1024*1024;
    rlimit rl = { 2*kOneGB, 2*kOneGB };
    setrlimit(RLIMIT_AS, &rl);
  }

  printf("pid = %d\n", getpid());

  char name[256] = { '\0' };
  strncpy(name, argv[0], sizeof name - 1);
  AsyncLogging log(::basename(name), kRollSize);
  log.start();
  g_asyncLog = &log;

  bool longLog = argc > 1;
  bench(longLog);
}
