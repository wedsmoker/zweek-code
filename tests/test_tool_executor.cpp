#include "tools/tool_executor.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace zweek::tools;

void TestFileOperations() {
  ToolExecutor executor;
  std::string test_dir = "test_env";
  
  // Clean up previous run
  if (fs::exists(test_dir)) {
    fs::remove_all(test_dir);
  }
  fs::create_directory(test_dir);
  
  executor.SetWorkingDirectory(test_dir);
  
  // Test WriteFile
  std::string filename = "test.txt";
  std::string content = "Hello, World!";
  bool success = executor.WriteFile(filename, content);
  assert(success);
  assert(fs::exists(fs::path(test_dir) / filename));
  
  // Test ReadFile
  std::string read_content = executor.ReadFile(filename);
  assert(read_content == content);
  
  // Test ListDir
  auto files = executor.ListDir(".");
  bool found = false;
  for (const auto& f : files) {
    if (f == filename) found = true;
  }
  assert(found);
  
  // Test Diff (Basic)
  std::string new_content = "Hello, Universe!";
  std::string diff = executor.GetDiff(filename, new_content);
  assert(!diff.empty());
  
  // Cleanup
  fs::remove_all(test_dir);
  
  std::cout << "TestFileOperations passed!" << std::endl;
}

int main() {
  TestFileOperations();
  return 0;
}
