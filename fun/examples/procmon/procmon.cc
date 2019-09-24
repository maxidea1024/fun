// TODO:
// - what if process exits?
//

// Represent parsed /proc/pid/stat
struct StatData
{
  void Parse(const char* startAtState, int kbPerPage)
  {
    // istringstream is probably not the most efficient way to Parse it,
    // see red-protorpc/examples/collect/ProcFs.cc for alternatives.
    std::istringstream iss(startAtState);

    //            0    1    2    3     4    5       6   7 8 9  11  13   15
    // 3770 (cat) R 3718 3770 3718 34818 3770 4202496 214 0 0 0 0 0 0 0 20
    // 16  18     19      20 21                   22      23      24              25
    //  0 1 0 298215 5750784 81 18446744073709551615 4194304 4242836 140736345340592
    //              26
    // 140736066274232 140575670169216 0 0 0 0 0 0 0 17 0 0 0 0 0 0

    iss >> state;
    iss >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags;
    iss >> minflt >> cminflt >> majflt >> cmajflt;
    iss >> utime >> stime >> cutime >> cstime;
    iss >> priority >> nice >> num_threads >> itrealvalue >> starttime;
    long vsize, rss;
    iss >> vsize >> rss >> rsslim;
    vsizeKb = vsize / 1024;
    rssKb = rss * kbPerPage;
  }
  // int pid;
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  int flags;

  long minflt;
  long cminflt;
  long majflt;
  long cmajflt;

  long utime;
  long stime;
  long cutime;
  long cstime;

  long priority;
  long nice;
  long num_threads;
  long itrealvalue;
  long starttime;

  long vsizeKb;
  long rssKb;
  long rsslim;
};

BOOST_STATIC_ASSERT(boost::is_pod<StatData>::value);

class Procmon : Noncopyable
{
 public:
  Procmon(EventLoop* loop, pid_t pid, uint16_t port, const char* procname)
    : kClockTicksPerSecond_(fun::ProcessInfo::clockTicksPerSecond()),
      kbPerPage_(fun::ProcessInfo::pageSize() / 1024),
      kBootTime_(getBootTime()),
      pid_(pid),
      server_(loop, InetAddress(port), getName()),
      procname_(procname ? procname : ProcessInfo::procname(readProcFile("stat")).as_string()),
      hostname_(ProcessInfo::hostname()),
      cmdline_(getCmdLine()),
      ticks_(0),
      cpu_usage_(600 / kPeriod_),  // 10 minutes
      cpu_chart_(640, 100, 600, kPeriod_),
      ram_chart_(640, 100, 7200, 30)
  {
    {
    // chdir to the same cwd of the process being monitored.
    String cwd = readLink("cwd");
    if (::chdir(cwd.c_str()))
    {
      LOG_SYSERR << "Cannot chdir() to " << cwd;
    }
    }

    {
    char cwd[1024];
    if (::getcwd(cwd, sizeof cwd))
    {
      LOG_INFO << "Current dir: " << cwd;
    }
    }
    bzero(&lastStatData_, sizeof lastStatData_);
    server_.SetHttpCallback(boost::bind(&Procmon::OnRequest, this, _1, _2));
  }

  void Start()
  {
    tick();
    server_.GetLoop()->ScheduleEvery(kPeriod_, boost::bind(&Procmon::tick, this));
    server_.Start();
  }

 private:

  String getName() const
  {
    char name[256];
    snprintf(name, sizeof name, "procmon-%d", pid_);
    return name;
  }

  void OnRequest(const HttpRequest& req, HttpResponse* resp)
  {
    resp->SetStatusCode(HttpResponse::k200Ok);
    resp->SetStatusMessage("OK");
    resp->SetContentType("text/plain");
    resp->AddHeader("Server", "Muduo-Procmon");

    /*
    if (!processExists(pid_))
    {
      resp->SetStatusCode(HttpResponse::k404NotFound);
      resp->SetStatusMessage("Not Found");
      resp->SetCloseConnection(true);
      return;
    }
    */

    if (req.path() == "/")
    {
      resp->SetContentType("text/html");
      fillOverview(req.query());
      resp->SetBody(response_.ReadAllAsString());
    }
    else if (req.path() == "/cmdline")
    {
      resp->SetBody(cmdline_);
    }
    else if (req.path() == "/cpu.png")
    {
      std::vector<double> cpu_usage;
      for (size_t i = 0; i < cpu_usage_.size(); ++i)
        cpu_usage.push_back(cpu_usage_[i].cpuUsage(kPeriod_, kClockTicksPerSecond_));
      String png = cpu_chart_.plotCpu(cpu_usage);
      resp->SetContentType("image/png");
      resp->SetBody(png);
    }
    // FIXME: replace with a map
    else if (req.path() == "/environ")
    {
      resp->SetBody(getEnviron());
    }
    else if (req.path() == "/io")
    {
      resp->SetBody(readProcFile("io"));
    }
    else if (req.path() == "/limits")
    {
      resp->SetBody(readProcFile("limits"));
    }
    else if (req.path() == "/maps")
    {
      resp->SetBody(readProcFile("maps"));
    }
    // numa_maps
    else if (req.path() == "/smaps")
    {
      resp->SetBody(readProcFile("smaps"));
    }
    else if (req.path() == "/status")
    {
      resp->SetBody(readProcFile("status"));
    }
    else if (req.path() == "/files")
    {
      listFiles();
      resp->SetBody(response_.ReadAllAsString());
    }
    else if (req.path() == "/threads")
    {
      listThreads();
      resp->SetBody(response_.ReadAllAsString());
    }
    else
    {
      resp->SetStatusCode(HttpResponse::k404NotFound);
      resp->SetStatusMessage("Not Found");
      resp->SetCloseConnection(true);
    }
  }

  void fillOverview(const String& query)
  {
    response_.DrainAll();
    Timestamp now = Timestamp::Now();
    appendResponse("<html><head><title>%s on %s</title>\n",
                   procname_.c_str(), hostname_.c_str());
    fillRefresh(query);
    appendResponse("</head><body>\n");

    String stat = readProcFile("stat");
    if (stat.empty())
    {
      appendResponse("<h1>PID %d doesn't exist.</h1></body></html>", pid_);
      return;
    }
    int pid = atoi(stat.c_str());
    fun_check(pid == pid_);
    StringPiece procname = ProcessInfo::procname(stat);
    appendResponse("<h1>%s on %s</h1>\n",
                   procname.as_string().c_str(), hostname_.c_str());
    response_.append("<p>Refresh <a href=\"?refresh=1\">1s</a> ");
    response_.append("<a href=\"?refresh=2\">2s</a> ");
    response_.append("<a href=\"?refresh=5\">5s</a> ");
    response_.append("<a href=\"?refresh=15\">15s</a> ");
    response_.append("<a href=\"?refresh=60\">60s</a>\n");
    response_.append("<p><a href=\"/cmdline\">Command line</a>\n");
    response_.append("<a href=\"/environ\">Environment variables</a>\n");
    response_.append("<a href=\"/threads\">Threads</a>\n");

    appendResponse("<p>Page generated at %s (UTC)", now.ToFormattedString().c_str());

    response_.append("<p><table>");
    StatData statData;  // how about use lastStatData_ ?
    bzero(&statData, sizeof statData);
    statData.Parse(procname.end()+1, kbPerPage_);  // end is ')'

    appendTableRow("PID", pid);
    Timestamp started(getStartTime(statData.starttime));  // FIXME: cache it;
    appendTableRow("Started at", started.ToFormattedString(false /*showMicroseconds*/) + " (UTC)");
    appendTableRowFloat("Uptime (s)", TimeDifference(now, started));  // FIXME: format as days+H:M:S
    appendTableRow("Executable", readLink("exe"));
    appendTableRow("Current dir", readLink("cwd"));

    appendTableRow("State", getState(statData.state));
    appendTableRowFloat("User time (s)", getSeconds(statData.utime));
    appendTableRowFloat("System time (s)", getSeconds(statData.stime));

    appendTableRow("VmSize (KiB)", statData.vsizeKb);
    appendTableRow("VmRSS (KiB)", statData.rssKb);
    appendTableRow("Threads", statData.num_threads);
    appendTableRow("CPU usage", "<img src=\"/cpu.png\" height=\"100\" witdh=\"640\">");

    appendTableRow("Priority", statData.priority);
    appendTableRow("Nice", statData.nice);

    appendTableRow("Minor page faults", statData.minflt);
    appendTableRow("Major page faults", statData.majflt);
    // TODO: user

    response_.append("</table>");
    response_.append("</body></html>");
  }

  void fillRefresh(const String& query)
  {
    size_t p = query.find("refresh=");
    if (p != String::npos)
    {
      int seconds = atoi(query.c_str()+p+8);
      if (seconds > 0)
      {
        appendResponse("<meta http-equiv=\"refresh\" content=\"%d\">\n", seconds);
      }
    }
  }

  static int dirFilter(const struct dirent* d)
  {
    return (d->d_name[0] != '.');
  }

  static char getDirType(char d_type)
  {
    switch (d_type)
    {
      case DT_REG: return '-';
      case DT_DIR: return 'd';
      case DT_LNK: return 'l';
      default: return '?';
    }
  }

  void listFiles()
  {
    struct dirent** namelist = NULL;
    int result = ::scandir(".", &namelist, dirFilter, alphasort);
    for (int i = 0; i < result; ++i)
    {
      struct stat stat;
      if (::lstat(namelist[i]->d_name, &stat) == 0)
      {
        Timestamp mtime(stat.st_mtime * Timestamp::kMicroSecondsPerSecond);
        appendResponse("%c %9ld %s %s", getDirType(namelist[i]->d_type), stat.st_size,
                       mtime.ToFormattedString(/*showMicroseconds=*/false).c_str(),
                       namelist[i]->d_name);
        if (namelist[i]->d_type == DT_LNK)
        {
          char link[1024];
          ssize_t len = ::readlink(namelist[i]->d_name, link, sizeof link - 1);
          if (len > 0)
          {
            link[len] = '\0';
            appendResponse(" -> %s", link);
          }
        }
        appendResponse("\n");
      }
      ::free(namelist[i]);
    }
    ::free(namelist);
  }

  void listThreads()
  {
    response_.DrainAll();
    // FIXME: implement this
  }

  String readProcFile(const char* basename)
  {
    char filename[256];
    snprintf(filename, sizeof filename, "/proc/%d/%s", pid_, basename);
    String content;
    FileUtil::readFile(filename, 1024*1024, &content);
    return content;
  }

  String readLink(const char* basename)
  {
    char filename[256];
    snprintf(filename, sizeof filename, "/proc/%d/%s", pid_, basename);
    char link[1024];
    ssize_t len = ::readlink(filename, link, sizeof link);
    String result;
    if (len > 0)
    {
      result.Assign(link, len);
    }
    return result;
  }

  int appendResponse(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

  void appendTableRow(const char* name, long value)
  {
    appendResponse("<tr><td>%s</td><td>%ld</td></tr>\n", name, value);
  }

  void appendTableRowFloat(const char* name, double value)
  {
    appendResponse("<tr><td>%s</td><td>%.2f</td></tr>\n", name, value);
  }

  void appendTableRow(const char* name, StringArg value)
  {
    appendResponse("<tr><td>%s</td><td>%s</td></tr>\n", name, value.c_str());
  }

  String getCmdLine()
  {
    return boost::replace_all_copy(readProcFile("cmdline"), String(1, '\0'), "\n\t");
  }

  String getEnviron()
  {
    return boost::replace_all_copy(readProcFile("environ"), String(1, '\0'), "\n");
  }

  Timestamp getStartTime(long starttime)
  {
    return Timestamp(Timestamp::kMicroSecondsPerSecond * kBootTime_
                     + Timestamp::kMicroSecondsPerSecond * starttime / kClockTicksPerSecond_);
  }

  double getSeconds(long ticks)
  {
    return static_cast<double>(ticks) / kClockTicksPerSecond_;
  }

  void tick()
  {
    String stat = readProcFile("stat");  // FIXME: catch file descriptor
    if (stat.empty())
      return;
    StringPiece procname = ProcessInfo::procname(stat);
    StatData statData;
    bzero(&statData, sizeof statData);
    statData.Parse(procname.end()+1, kbPerPage_);  // end is ')'
    if (ticks_ > 0)
    {
      CpuTime time;
      time.userTime_ = std::max(0, static_cast<int>(statData.utime - lastStatData_.utime));
      time.sysTime_ = std::max(0, static_cast<int>(statData.stime - lastStatData_.stime));
      cpu_usage_.push_back(time);
    }

    lastStatData_ = statData;
    ++ticks_;
  }

  //
  // static member functions
  //

  static const char* getState(char state)
  {
    // One character from the String "RSDZTW" where R is running, S is sleeping in
    // an interruptible wait, D is waiting in uninterruptible disk sleep, Z is zombie,
    // T is traced or stopped (on a signal), and W is paging.
    switch (state)
    {
      case 'R':
        return "Running";
      case 'S':
        return "Sleeping";
      case 'D':
        return "Disk sleep";
      case 'Z':
        return "Zombie";
      default:
        return "Unknown";
    }
  }

  static long getLong(const String& status, const char* key)
  {
    long result = 0;
    size_t pos = status.find(key);
    if (pos != String::npos)
    {
      result = ::atol(status.c_str() + pos + strlen(key));
    }
    return result;
  }

  static long getBootTime()
  {
    String stat;
    FileUtil::readFile("/proc/stat", 65536, &stat);
    return getLong(stat, "btime ");
  }

  struct CpuTime
  {
    int userTime_;
    int sysTime_;
    double cpuUsage(double kPeriod, double kClockTicksPerSecond) const
    {
      return (userTime_ + sysTime_) / (kClockTicksPerSecond * kPeriod);
    }
  };

  const static int kPeriod_ = 2.0;
  const int kClockTicksPerSecond_;
  const int kbPerPage_;
  const long kBootTime_;  // in Unix-time
  const pid_t pid_;
  HttpServer server_;
  const String procname_;
  const String hostname_;
  const String cmdline_;
  int ticks_;
  StatData lastStatData_;
  boost::circular_buffer<CpuTime> cpu_usage_;
  Plot cpu_chart_;
  Plot ram_chart_;
  // scratch variables
  Buffer response_;
};

// define outline for __attribute__
int Procmon::appendResponse(const char* fmt, ...)
{
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);
  response_.append(buf);
  return ret;
}

bool processExists(pid_t pid)
{
  char filename[256];
  snprintf(filename, sizeof filename, "/proc/%d/stat", pid);
  return ::access(filename, R_OK) == 0;
}

int main(int argc, char* argv[])
{
  if (argc < 3) {
    printf("Usage: %s pid port [name]\n", argv[0]);
    return 0;
  }

  int pid = atoi(argv[1]);
  if (!processExists(pid)) {
    printf("Process %d doesn't exist.\n", pid);
    return 1;
  }

  EventLoop loop;
  uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
  Procmon procmon(&loop, pid, port, argc > 3 ? argv[3] : NULL);
  procmon.Start();
  loop.Loop();
}
