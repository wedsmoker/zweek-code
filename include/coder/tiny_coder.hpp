#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include "models/model_loader.hpp"

namespace zweek {
namespace coder {

struct CodeEdit {
  std::string file_path;
  std::string original_snippet;
  std::string new_content;
  std::string explanation;
};

class TinyCoder {
public:
  TinyCoder();
  ~TinyCoder();

  // Load the tiny coder model (StarCoder)
  bool LoadModel(const std::string &model_path);
  void UnloadModel();

  // Generate code edits based on a plan and file context
  std::vector<CodeEdit> GenerateEdits(const std::string &instruction,
                                      const std::vector<std::string> &files,
                                      std::function<void(const std::string &)> stream_callback,
                                      std::atomic<bool>* interrupt_flag = nullptr);

private:
  zweek::models::ModelLoader model_loader_;
  bool model_loaded_ = false;

  std::string ConstructPrompt(const std::string &instruction,
                              const std::vector<std::string> &files);
};

} // namespace coder
} // namespace zweek
