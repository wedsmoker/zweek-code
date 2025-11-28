#include "chat/chat_mode.hpp"

namespace zweek {
namespace chat {

ChatMode::ChatMode() {}
ChatMode::~ChatMode() { UnloadModel(); }

bool ChatMode::LoadModel(const std::string &model_path) {
  model_loaded_ = model_loader_.Load(model_path, 2048);
  return model_loaded_;
}

void ChatMode::UnloadModel() {
  model_loader_.Unload();
  model_loaded_ = false;
}

void ChatMode::ClearHistory() { history_.clear(); }

std::string ChatMode::Chat(const std::string &user_message,
                           const std::vector<std::string> &context_files,
                           std::function<void(const std::string &)> stream_callback,
                           std::atomic<bool>* interrupt_flag) {
  if (!model_loaded_) {
    LoadModel("models/Qwen3-0.6B-Q8_0.gguf");
  }

  if (!model_loaded_) {
    return "Error: Chat model not loaded";
  }

  // Use ChatML format for Qwen3 with thinking trigger
  std::string prompt = 
    "<|im_start|>system\n"
    "You are a helpful coding assistant.<|im_end|>\n"
    "<|im_start|>user\n" + 
    user_message + "<|im_end|>\n"
    "<|im_start|>assistant\n"
    "<|im_start|>think\n";

  // Increased max tokens to 2048 to prevent cutoff
  std::string response = model_loader_.Infer(prompt, "", 2048, stream_callback, interrupt_flag);

  history_.push_back({"user", user_message});
  history_.push_back({"assistant", response});

  return response;
}

} // namespace chat
} // namespace zweek
