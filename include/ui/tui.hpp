#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <string>
#include <vector>


namespace zweek {

// Forward declarations
namespace commands {
  class CommandHandler;
}

namespace ui {

enum class PipelineStage {
  Idle,
  Planning,
  ToolExecution,
  CodeDrafting,
  StyleEnforcing,
  ComplexityAuditing,
  GatekeeperReview,
  Complete,
  Error
};

enum class Mode {
  Plan, // Show plan, wait for approval
  Auto  // Auto-apply edits
};

struct TUIState {
  std::string user_input;
  Mode current_mode = Mode::Plan;
  PipelineStage current_stage = PipelineStage::Idle;
  float progress = 0.0f;
  std::string status_message;
  std::string code_preview;
  std::string quality_report;
  bool show_diff = false;
  std::vector<std::string> conversation_history;
  int scroll_position = 0; // For manual scrolling (-1 = sticky bottom)
  std::vector<std::string> command_history; // Previous submitted commands
  int history_index = -1; // Current position in history (-1 = not browsing)
  std::atomic<bool> interrupt_inference_{false}; // Set to true to interrupt model
  
  // Thinking section support
  std::string current_thinking;  // Buffer for thinking content
  std::string current_answer;    // Buffer for final answer
  bool in_thinking_section = true;  // Track which section we're in
  bool show_thinking = true;     // Toggle thinking visibility
  int spinner_frame = 0;         // Spinner animation frame
  std::string current_directory; // Current working directory
  
  // Command autocomplete
  std::vector<std::string> command_suggestions;
  int suggestion_index = -1;
  bool show_suggestions = false;
};

class TUI {
public:
  TUI();
  ~TUI();

  // Main event loop
  void Run();

  // Update state from pipeline
  void UpdateStage(PipelineStage stage, float progress);
  void SetCodePreview(const std::string &code);
  void SetQualityReport(const std::string &report);
  void SetError(const std::string &error);
  void AddToHistory(const std::string &message);
  void AppendToLastMessage(const std::string &chunk);
  void SetCurrentDirectory(const std::string &path);

  // Mode switching
  void SetMode(Mode mode);
  Mode GetMode() const { return state_.current_mode; }

  // Callbacks for user actions
  void SetOnSubmit(std::function<void(const std::string &)> callback);
  void SetOnAccept(std::function<void()> callback);
  void SetOnReject(std::function<void()> callback);
  void SetOnModify(std::function<void()> callback);
  void SetOnModeSwitch(std::function<void(Mode)> callback);
  
  // Set command handler for autocomplete
  void SetCommandHandler(zweek::commands::CommandHandler* cmd_handler) {
    command_handler_ = cmd_handler;
  }
  
  // Access to state (for interrupt flag)
  TUIState& GetState() { return state_; }

private:
  ftxui::Component CreateLayout();
  ftxui::Component CreateTerminalView();
  ftxui::Component CreateModeSelector();
  ftxui::Component CreateInputLine();

  std::string StageToString(PipelineStage stage);
  std::string ModeToString(Mode mode);

  TUIState state_;
  ftxui::ScreenInteractive screen_;

  // Callbacks
  std::function<void(const std::string &)> on_submit_;
  std::function<void()> on_accept_;
  std::function<void()> on_reject_;
  std::function<void()> on_modify_;
  std::function<void(Mode)> on_mode_switch_;
  
  // Command handler for autocomplete
  zweek::commands::CommandHandler* command_handler_ = nullptr;
};

} // namespace ui
} // namespace zweek
