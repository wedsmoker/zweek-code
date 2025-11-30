#pragma once

#include <functional>
#include <string>

namespace zweek {

// Forward declaration
namespace history {
  class HistoryManager;
}
namespace chat {
  class ChatMode;
}

namespace commands {

// Command handler result
struct CommandResult {
  bool handled = false;
  std::string response;
};

// Command system
class CommandHandler {
public:
  CommandHandler();

  // Check if input is a command and handle it
  CommandResult HandleCommand(const std::string &input);

  // Set history manager for history commands
  void SetHistoryManager(history::HistoryManager* history_mgr) { 
    history_manager_ = history_mgr; 
  }

  // Set chat mode for session reloading
  void SetChatMode(chat::ChatMode* chat_mode) {
    chat_mode_ = chat_mode;
  }

private:
  std::string GetHelpText();
  
  history::HistoryManager* history_manager_ = nullptr;
  chat::ChatMode* chat_mode_ = nullptr;
  std::vector<std::string> cached_sessions_;
};

} // namespace commands
} // namespace zweek
