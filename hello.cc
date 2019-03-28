#include <stdio.h>

class Application : public fun::ServerApplication {
 public:
  Application() {
    // Do something...
  }

  bool Initialize() override {
    if (!fun::ServerApplication::Initialize()) {
      return false;
    }

    json::Value json;
    if (json.LoadFromFile(filename)) {
      server.LoadConfigFromJson(json);
    } else {
      server.LoadDefaultConfig();
    }

    return true;
  }

  void Close() override {
    // do something...

    fun::ServerApplication::Close();
  }

 private:
  fun::Server server;
}

int main(int argc, char* argv) {
  Application app;
  if (!app.Initialize(argc, argv)) {
    return -1;
  }
  app.Run();
  return 0;
}
