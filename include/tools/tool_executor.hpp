#pragma once

#include <string>
#include <vector>

namespace zweek {
namespace tools {

// Tool executor interface
// Will handle file operations and shell execution in Phase 2

class ToolExecutor {
public:
  ToolExecutor();
  ~ToolExecutor();

  // Set working directory
  void SetWorkingDirectory(const std::string &path);
  
  // Get current working directory
  std::string GetWorkingDirectory() const { return working_dir_; }

  // File operations
  std::string ReadFile(const std::string &path);
  bool WriteFile(const std::string &path, const std::string &content);
  std::vector<std::string> ListDir(const std::string &path);
  
  // Diff generation
  std::string GetDiff(const std::string &path, const std::string &new_content);

private:
  std::string working_dir_ = ".";
  
  // Helper to resolve path relative to working dir
  std::string ResolvePath(const std::string &path);
};

} // namespace tools
} // namespace zweek
