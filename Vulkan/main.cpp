#include "Application.h"
#include "Log.h"

#include <memory>

int main(const int argc, const char* argv[]) {
   Vulkan::Log::Init();
   try {
      std::unique_ptr<Vulkan::Application> app = CreateApplication(argc, argv);
      app->Run();
   } catch (std::exception err) {
      CORE_LOG_FATAL(err.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
