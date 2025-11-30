#include "tools/tool_executor.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace zweek {
namespace tools {

namespace fs = std::filesystem;

ToolExecutor::ToolExecutor() {}

ToolExecutor::~ToolExecutor() {}

void ToolExecutor::SetWorkingDirectory(const std::string &path) {
  if (fs::exists(path) && fs::is_directory(path)) {
    working_dir_ = path;
  } else {
    // If path doesn't exist, keep default or log error
    // For now, just keep default "."
  }
}

std::string ToolExecutor::ResolvePath(const std::string &path) {
  fs::path p(path);
  if (p.is_absolute()) {
    return p.string();
  }
  return (fs::path(working_dir_) / p).string();
}

std::string ToolExecutor::ReadFile(const std::string &path) {
  std::string full_path = ResolvePath(path);
  std::ifstream file(full_path);
  if (!file.is_open()) {
    return ""; // Or throw exception / return error code
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

bool ToolExecutor::WriteFile(const std::string &path, const std::string &content) {
  std::string full_path = ResolvePath(path);
  // Ensure directory exists
  fs::create_directories(fs::path(full_path).parent_path());
  
  std::ofstream file(full_path);
  if (!file.is_open()) {
    return false;
  }
  file << content;
  return true;
}

std::vector<std::string> ToolExecutor::ListDir(const std::string &path) {
  std::string full_path = ResolvePath(path);
  std::vector<std::string> files;
  
  if (!fs::exists(full_path) || !fs::is_directory(full_path)) {
    return files;
  }

  for (const auto &entry : fs::directory_iterator(full_path)) {
    files.push_back(entry.path().filename().string());
  }
  return files;
}

std::string ToolExecutor::GetDiff(const std::string &path, const std::string &new_content) {
  std::string original = ReadFile(path);
  
  // Simple line-based diff (very basic implementation)
  // In a real scenario, use a diff library or git
  
  std::stringstream ss_orig(original);
  std::stringstream ss_new(new_content);
  std::string line_orig, line_new;
  
  std::string diff_output;
  
  // This is a placeholder for a real diff algorithm
  // For now, we just show that it's different
  if (original != new_content) {
    diff_output = "--- " + path + "\n+++ " + path + "\n";
    diff_output += "@@ -1 +1 @@\n";
    diff_output += "- " + original.substr(0, std::min((size_t)50, original.length())) + "...\n";
    diff_output += "+ " + new_content.substr(0, std::min((size_t)50, new_content.length())) + "...\n";
  }
  
  return diff_output;
}

} // namespace tools
} // namespace zweek
