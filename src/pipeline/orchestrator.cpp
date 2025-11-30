#include "pipeline/orchestrator.hpp"
#include "commands/command_handler.hpp"

namespace zweek {
namespace pipeline {

Orchestrator::Orchestrator() : command_handler_() {
  // Initialize history manager
  history_manager_.Init("");
  
  // Wire history manager to chat mode
  chat_mode_.SetHistoryManager(&history_manager_);
  
  // Wire history manager to command handler
  command_handler_.SetHistoryManager(&history_manager_);
  command_handler_.SetChatMode(&chat_mode_);
  
  // Wire tool executor to command handler
  command_handler_.SetToolExecutor(&tool_executor_);
  
  // Wire directory change callback
  command_handler_.SetDirectoryChangeCallback([this](const std::string& path) {
    if (directory_update_callback_) {
      directory_update_callback_(path);
    }
  });
}

Orchestrator::~Orchestrator() {
  // Destructor
}

void Orchestrator::SetWorkingDirectory(const std::string &path) {
  tool_executor_.SetWorkingDirectory(path);
  if (directory_update_callback_) {
      directory_update_callback_(path);
  }
}

void Orchestrator::ProcessRequest(const std::string &user_request) {
  // Check if it's a command first
  auto cmd_result = command_handler_.HandleCommand(user_request);
  if (cmd_result.handled) {
    if (response_callback_) {
      response_callback_(cmd_result.response);
    }
    return;
  }

  if (progress_callback_) {
    progress_callback_("Classifying intent...");
  }

  // Step 1: Classify intent
  Intent intent = router_.ClassifyIntent(user_request);
  WorkflowType workflow = router_.GetWorkflow(intent);

  // Step 2: Execute appropriate workflow
  switch (workflow) {
  case WorkflowType::CodePipeline:
    if (progress_callback_) {
      progress_callback_("Starting code generation pipeline...");
    }
    RunCodePipeline(user_request);
    break;

  case WorkflowType::ChatMode:
    if (progress_callback_) {
      progress_callback_("Entering chat mode...");
    }
    RunChatMode(user_request);
    break;

  case WorkflowType::ToolMode:
    if (progress_callback_) {
      progress_callback_("Running tools...");
    }
    RunToolMode(user_request);
    break;
  }
}

void Orchestrator::SetProgressCallback(
    std::function<void(const std::string &)> callback) {
  progress_callback_ = callback;
}

void Orchestrator::SetResponseCallback(
    std::function<void(const std::string &)> callback) {
  response_callback_ = callback;
}

void Orchestrator::SetStreamCallback(
    std::function<void(const std::string &)> callback) {
  stream_callback_ = callback;
}

void Orchestrator::SetDirectoryUpdateCallback(
    std::function<void(const std::string &)> callback) {
  directory_update_callback_ = callback;
}

void Orchestrator::RunCodePipeline(const std::string &request) {
  // TODO: Implement 5-model pipeline
  // For now, just mock it
  if (progress_callback_) {
    progress_callback_("[PLAN] Analyzing request...");
  }

  if (response_callback_) {
    response_callback_("Code generation coming in Phase 2!");
  }
}

void Orchestrator::RunChatMode(const std::string &request) {
  // Use ChatMode to respond
  std::vector<std::string> context; // TODO: Get relevant files
  
  std::string response = chat_mode_.Chat(request, context, [&](const std::string& chunk) {
    if (stream_callback_) {
      stream_callback_(chunk);
    }
  }, interrupt_flag_);

  // Mark as complete after streaming finishes
  if (response_callback_) {
    response_callback_(response);
  }
}

void Orchestrator::RunToolMode(const std::string &request) {
  // TODO: Implement deterministic tools (grep, git, etc.)
  if (response_callback_) {
    response_callback_("Tool mode coming in Phase 2!");
  }
}

} // namespace pipeline
} // namespace zweek
