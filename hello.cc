//vscode
//  tab:2
//  tab_as_spaces: yes

#include <stdio.h>

/**
 * Server Application
 */
class Application : public fun::ServerApplication {
 public:
  /**
   * Default constructor.
   */
  Application() {
    // Do something...
  }

  /**
   * Initialize server application.
   */
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

  /**
   * Close server application.
   */
  void Close() override {
    // do something...

    fun::ServerApplication::Close();
  }

 private:
  /// Server object.
  fun::Server server;
}


int main(int argc, char* argv[]) {
  Application app;

  // Initialize server application.
  if (!app.Initialize(argc, argv)) {
    return -1;
  }

  // Run main loop.
  app.Run();

  return 0;
}
