#pragma once

#include "models/model_loader.hpp"
#include <string>
#include <vector>
#include <atomic>


namespace zweek {

// Forward declaration
namespace history {
  class HistoryManager;
}

namespace chat {

// Chat message structure
struct Message {
  std::string role; // "user" or "assistant"
  std::string content;
};

// Chat mode handler
class ChatMode {
public:
  ChatMode();
  ~ChatMode();

  // Load chat model (TinyLlama-Chat)
  bool LoadModel(const std::string &model_path);

  // Unload to free memory
  void UnloadModel();
  
  // Set history manager for persistence (optional)
  void SetHistoryManager(history::HistoryManager* history_mgr) { 
    history_manager_ = history_mgr; 
  }

  // Chat with context
  std::string Chat(const std::string &user_message,
                   const std::vector<std::string> &context_files,
                   std::function<void(const std::string &)> stream_callback,
                   std::atomic<bool>* interrupt_flag = nullptr);

  // Get conversation history
  const std::vector<Message> &GetHistory() const { return history_; }

  // Clear conversation
  void ClearHistory();
  
  // Load history from persistence (if available)
  void LoadSessionHistory();

private:
  bool model_loaded_ = false;
  std::vector<Message> history_;
  models::ModelLoader model_loader_;
  history::HistoryManager* history_manager_ = nullptr;
};

} // namespace chat
} // namespace zweek
