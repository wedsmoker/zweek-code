#include "ui/tui.hpp"
#include "ui/branding.hpp"
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace zweek {
namespace ui {

TUI::TUI() : screen_(ScreenInteractive::Fullscreen()) {
  state_.status_message = "Ready";
  state_.conversation_history.push_back(
      "Welcome to Zweek Code - Local AI Coding Assistant");
  state_.conversation_history.push_back("Type your request and press Enter...");
  state_.conversation_history.push_back("");
  state_.scroll_position = -1; // Start with sticky scrolling enabled
}

TUI::~TUI() = default;

void TUI::Run() {
  auto layout = CreateLayout();
  screen_.Loop(layout);
}

void TUI::UpdateStage(PipelineStage stage, float progress) {
  state_.current_stage = stage;
  state_.progress = progress;
  state_.status_message = StageToString(stage);
  state_.conversation_history.push_back("[" + StageToString(stage) + "]");
  screen_.PostEvent(Event::Custom);
}

void TUI::SetCodePreview(const std::string &code) {
  state_.code_preview = code;
  state_.conversation_history.push_back("");
  state_.conversation_history.push_back("Generated code:");
  state_.conversation_history.push_back(code);
  screen_.PostEvent(Event::Custom);
}

void TUI::SetQualityReport(const std::string &report) {
  state_.quality_report = report;
  state_.conversation_history.push_back("Quality: " + report);
  screen_.PostEvent(Event::Custom);
}

void TUI::SetError(const std::string &error) {
  state_.current_stage = PipelineStage::Error;
  state_.status_message = error;
  state_.conversation_history.push_back("Error: " + error);
  screen_.PostEvent(Event::Custom);
}

void TUI::AddToHistory(const std::string &message) {
  // Split multi-line messages into separate entries
  std::istringstream stream(message);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      state_.conversation_history.push_back(line);
    }
  }
  screen_.PostEvent(Event::Custom);
}

void TUI::AppendToLastMessage(const std::string &chunk) {
  // Detect if we're transitioning from thinking to answer
  static std::string buffer;
  buffer += chunk;
  
  // Check for Qwen3 thinking end marker: </think>
  size_t marker_pos = buffer.find("</think>");
  if (marker_pos != std::string::npos && state_.in_thinking_section) {
    // Found the transition marker
    state_.in_thinking_section = false;
    
    // Everything before marker is thinking
    state_.current_thinking += buffer.substr(0, marker_pos);
    
    // Skip the marker itself and get answer content
    // +8 is length of "</think>", also skip potential newline after it
    size_t content_start = marker_pos + 8;
    if (content_start < buffer.length() && buffer[content_start] == '\n') {
        content_start++;
    }
    buffer = buffer.substr(content_start);
  }
  
  // Append to appropriate buffer
  if (state_.in_thinking_section) {
    // Only append if we haven't found the marker yet
    // If we have a partial marker at the end, we might need to wait, 
    // but for now simple appending is safer to avoid dropping text.
    // The previous logic handles the split when the full marker arrives.
    // However, we need to be careful not to double-add if we just processed the marker above.
    // Since we clear/adjust 'buffer' above, we should only append 'chunk' if we didn't just switch.
    
    // Actually, the logic above uses 'buffer' which accumulates. 
    // We should NOT append 'chunk' again if we are using 'buffer'.
    // Let's rewrite this to be cleaner using just the buffer.
    
    // If we are still in thinking section, we just keep accumulating in buffer
    // We don't push to current_thinking until we confirm it's NOT part of the marker
    // But for streaming responsiveness, we want to push safe chars.
    
    // Simplified approach:
    // If we switched sections, 'buffer' now contains the start of the answer.
    // If we didn't switch, 'buffer' contains thinking + maybe partial marker.
    
    if (state_.in_thinking_section) {
       // We are still in thinking mode. 
       // To be safe and responsive, we can push everything except the last few chars 
       // that might be start of </think>
       // But simpler: just push to current_thinking, and if we find marker later, 
       // we fix it up.
       
       // Wait, the previous logic was:
       // 1. buffer += chunk
       // 2. check marker
       // 3. if found -> split
       // 4. append chunk to current section
       
       // The bug was step 4 appended 'chunk' blindly even if we just extracted it from 'buffer'.
       // We should use 'buffer' to drive the state, but 'buffer' is static!
       // Static buffer is dangerous if we have multiple calls. 
       // Let's fix the logic to be robust.
    }
  } 
  
  // CORRECTED LOGIC:
  // We already added chunk to buffer.
  // If we found the marker, we extracted thinking and updated buffer to be answer.
  // If we didn't find marker, buffer is all thinking (so far).
  
  if (!state_.in_thinking_section) {
      // We are in answer mode.
      state_.current_answer += buffer;
      buffer.clear();
  } else {
      // We are in thinking mode.
      // To keep UI responsive, we want to move confirmed text from buffer to current_thinking.
      // Confirmed text is everything up to where a partial "</think>" could start.
      // </think> is 8 chars.
      if (buffer.length() > 8) {
          size_t safe_len = buffer.length() - 8;
          state_.current_thinking += buffer.substr(0, safe_len);
          buffer = buffer.substr(safe_len);
      }
      // If buffer is small, keep it there to check for marker next time
  }
  
  screen_.PostEvent(Event::Custom);
}

void TUI::SetMode(Mode mode) {
  state_.current_mode = mode;
  state_.conversation_history.push_back("Switched to " + ModeToString(mode) +
                                        " mode");
  if (on_mode_switch_) {
    on_mode_switch_(mode);
  }
  screen_.PostEvent(Event::Custom);
}

void TUI::SetOnSubmit(std::function<void(const std::string &)> callback) {
  on_submit_ = callback;
}

void TUI::SetOnAccept(std::function<void()> callback) { on_accept_ = callback; }

void TUI::SetOnReject(std::function<void()> callback) { on_reject_ = callback; }

void TUI::SetOnModify(std::function<void()> callback) { on_modify_ = callback; }

void TUI::SetOnModeSwitch(std::function<void(Mode)> callback) {
  on_mode_switch_ = callback;
}

Component TUI::CreateLayout() {
  auto terminal_view = CreateTerminalView();
  auto mode_selector = CreateModeSelector();
  auto input_line = CreateInputLine();

  auto main_container =
      Container::Vertical({terminal_view, mode_selector, input_line});

  return Renderer(main_container, [=] {
    return vbox({terminal_view->Render() | flex, separator(),
                 mode_selector->Render(), input_line->Render()});
  });
}

Component TUI::CreateTerminalView() {
  auto renderer = Renderer([this] {
    // Build conversation history display
    Elements history_elements;

    // Show logo at top - split into lines
    std::string logo = ZWEEK_LOGO;
    std::istringstream logo_stream(logo);
    std::string line;
    while (std::getline(logo_stream, line)) {
      if (!line.empty()) {
        history_elements.push_back(text(line) | color(Color::Cyan));
      }
    }

    history_elements.push_back(text(VERSION + " | " + TAGLINE) |
                               color(Color::GrayLight) | dim);
    history_elements.push_back(text(""));

    // Show conversation history (all messages)
    int total_messages = state_.conversation_history.size();
    
    // Determine target scroll position
    int render_pos = state_.scroll_position;
    
    // Calculate total lines for clamping/sticky logic
    int total_lines = history_elements.size() + total_messages;
    
    // Add thinking/answer sections if present
    if (!state_.current_thinking.empty() || !state_.current_answer.empty()) {
        total_lines += 3; // Thinking header + content/collapsed + answer
    }
    
    if (state_.current_stage != PipelineStage::Idle &&
        state_.current_stage != PipelineStage::Complete) {
        total_lines += 2; // For spinner
    }

    // Sticky scrolling: if -1, jump to bottom
    if (state_.scroll_position == -1) {
        render_pos = total_lines - 1;
    }

    for (int i = 0; i < total_messages; i++) {
      const auto &msg = state_.conversation_history[i];
      Element e;

      // Color code based on message type
      if (msg.find("Error:") == 0) {
        e = text(msg) | color(Color::Red);
      } else if (msg.find("[") == 0 && msg.find("]") != std::string::npos) {
        // Status messages
        e = text(msg) | color(Color::Yellow) | dim;
      } else if (msg.find("Generated code:") == 0) {
        e = text(msg) | color(Color::Green) | bold;
      } else if (msg.find("Quality:") == 0) {
        e = text(msg) | color(Color::Cyan);
      } else if (msg.find("Switched to") == 0) {
        e = text(msg) | color(Color::Magenta);
      } else if (msg.find(">") == 0) {
        // User input
        e = text(msg) | color(Color::White) | bold;
      } else {
        e = text(msg);
      }

      // Apply focus to the element at the current scroll position
      if (history_elements.size() == render_pos) {
          e = e | focus;
      }
      history_elements.push_back(e);
    }
    
    // Show thinking and answer sections if streaming
    if (!state_.current_thinking.empty() || !state_.current_answer.empty()) {
      history_elements.push_back(text(""));
      
      // Thinking section (collapsible)
      if (!state_.current_thinking.empty()) {
        if (state_.show_thinking) {
          Element header = text("▼ Thinking (press 't' to hide)") | color(Color::GrayDark) | dim;
          if (history_elements.size() == render_pos) header = header | focus;
          history_elements.push_back(header);
          
          // Split thinking into lines and render dimmed
          std::istringstream thinking_stream(state_.current_thinking);
          std::string thinking_line;
          while (std::getline(thinking_stream, thinking_line)) {
            Element line = text(thinking_line) | color(Color::GrayLight) | dim;
            if (history_elements.size() == render_pos) line = line | focus;
            history_elements.push_back(line);
          }
        } else {
          Element header = text("▶ Thinking (press 't' to show)") | color(Color::GrayDark) | dim;
          if (history_elements.size() == render_pos) header = header | focus;
          history_elements.push_back(header);
        }
      }
      
      // Answer section
      if (!state_.current_answer.empty()) {
        history_elements.push_back(text(""));
        Element header = text("Final Answer:") | color(Color::Green) | bold;
        if (history_elements.size() == render_pos) header = header | focus;
        history_elements.push_back(header);
        
        // Split answer into lines
        std::istringstream answer_stream(state_.current_answer);
        std::string answer_line;
        while (std::getline(answer_stream, answer_line)) {
          Element line = text(answer_line);
          if (history_elements.size() == render_pos) line = line | focus;
          history_elements.push_back(line);
        }
      }
    }

    // Show spinner if processing
    if (state_.current_stage != PipelineStage::Idle &&
        state_.current_stage != PipelineStage::Complete) {
      const std::vector<std::string> spinner_chars = 
        {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
      auto spinner = spinner_chars[state_.spinner_frame % 10];
      
      history_elements.push_back(text(""));
      auto status_bar = hbox({
        text("Working... ") | color(Color::Yellow),
        text(spinner) | color(Color::Yellow) | bold
      });
      
      if (history_elements.size() == render_pos) {
          status_bar = status_bar | focus;
      }
      history_elements.push_back(status_bar);
    }
    
    // Safety clamp (in case we are not in sticky mode but out of bounds)
    if (render_pos >= history_elements.size() && !history_elements.empty()) {
        // We can't easily change the focus of already pushed elements without rebuilding
        // But since we render every frame, next frame will be correct if we update state
        // For this frame, just focus the last one
        history_elements.back() = history_elements.back() | focus;
    }

    return vbox(history_elements) | vscroll_indicator | frame | flex;
  });

  return CatchEvent(renderer, [this](Event event) {
    // Calculate total lines to handle bounds
    int logo_lines = 0;
    std::string logo = ZWEEK_LOGO;
    std::istringstream logo_stream(logo);
    std::string line;
    while (std::getline(logo_stream, line)) { if(!line.empty()) logo_lines++; }
    logo_lines += 2; // Version + empty line

    int total_lines = logo_lines + state_.conversation_history.size();
    
    // Add thinking/answer sections if present
    if (!state_.current_thinking.empty()) {
        // Count thinking lines
        total_lines += 1; // Header
        if (state_.show_thinking) {
            std::istringstream ts(state_.current_thinking);
            std::string tline;
            while (std::getline(ts, tline)) total_lines++;
        }
    }
    if (!state_.current_answer.empty()) {
        // Count answer lines
        total_lines += 2; // Empty + "Final Answer:" header
        std::istringstream as(state_.current_answer);
        std::string aline;
        while (std::getline(as, aline)) total_lines++;
    }
    
    if (state_.current_stage != PipelineStage::Idle &&
        state_.current_stage != PipelineStage::Complete) {
        total_lines += 2;
    }

    auto scroll_up = [&](int amount) {
        if (state_.scroll_position == -1) {
            state_.scroll_position = total_lines - 1;
        }
        state_.scroll_position -= amount;
        if (state_.scroll_position < 0) state_.scroll_position = 0;
    };

    auto scroll_down = [&](int amount) {
        if (state_.scroll_position == -1) return; // Already at bottom
        
        state_.scroll_position += amount;
        if (state_.scroll_position >= total_lines - 1) {
            state_.scroll_position = -1; // Re-enable sticky
        }
    };

    if (event.is_mouse()) {
        if (event.mouse().button == Mouse::WheelUp) {
            scroll_up(3);
            return true;
        }
        if (event.mouse().button == Mouse::WheelDown) {
            scroll_down(3);
            return true;
        }
    }
    
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        scroll_up(1);
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        scroll_down(1);
        return true;
    }
    if (event == Event::PageUp) {
        scroll_up(10);
        return true;
    }
    if (event == Event::PageDown) {
        scroll_down(10);
        return true;
    }
    if (event == Event::Home) {
        state_.scroll_position = 0;
        return true;
    }
    if (event == Event::End) {
        state_.scroll_position = -1;
        return true;
    }

    return false;
  });
}

Component TUI::CreateModeSelector() {
  return Renderer([this] {
    std::string mode_text = "Mode: ";
    if (state_.current_mode == Mode::Plan) {
      mode_text += "[Plan] Auto  (press 'm' to switch)";
    } else {
      mode_text += "Plan [Auto]  (press 'm' to switch)";
    }

    std::string help_text = "Ctrl+C: Exit";
    if (state_.current_stage == PipelineStage::Complete) {
      help_text = "y: Accept | n: Reject | Ctrl+C: Exit";
    }

    return hbox({text(mode_text) | color(Color::Cyan), separator(),
                 text(help_text) | dim});
  });
}

Component TUI::CreateInputLine() {
  InputOption input_opt;
  input_opt.multiline = false;
  input_opt.placeholder = "Type your request...";
  input_opt.on_enter = [this] {
    if (!state_.user_input.empty()) {
      state_.conversation_history.push_back("> " + state_.user_input);
      // Add to command history
      state_.command_history.push_back(state_.user_input);
      state_.history_index = -1; // Reset history browsing
      
      // Reset thinking/answer buffers for new request
      state_.current_thinking.clear();
      state_.current_answer.clear();
      state_.in_thinking_section = true;
      
      if (on_submit_) {
        on_submit_(state_.user_input);
      }
      state_.user_input.clear();
    }
  };

  auto input_box = Input(&state_.user_input, input_opt);

  // Simple keyboard shortcuts (press 'm' to switch modes, 'y' to accept, 'n' to
  // reject)
  auto input_with_hotkeys = CatchEvent(input_box, [this](Event event) {
    // ESC key - interrupt model generation
    if (event == Event::Escape) {
      state_.interrupt_inference_.store(true);
      state_.conversation_history.push_back("[Interrupting...]");
      return true;
    }
    // Up arrow - navigate to previous command in history
    if (event == Event::ArrowUp) {
      if (!state_.command_history.empty()) {
        if (state_.history_index == -1) {
          // Start browsing from most recent
          state_.history_index = state_.command_history.size() - 1;
        } else if (state_.history_index > 0) {
          state_.history_index--;
        }
        state_.user_input = state_.command_history[state_.history_index];
        return true;
      }
    }
    // Down arrow - navigate to next command in history
    if (event == Event::ArrowDown) {
      if (state_.history_index != -1) {
        if (state_.history_index < state_.command_history.size() - 1) {
          state_.history_index++;
          state_.user_input = state_.command_history[state_.history_index];
        } else {
          // End of history, clear input
          state_.history_index = -1;
          state_.user_input.clear();
        }
        return true;
      }
    }
    // Press 'm' to switch mode
    if (event.is_character() && event.character() == "m") {
      // Only switch if input is empty (so typing 'm' in requests still works)
      if (state_.user_input.empty()) {
        SetMode(state_.current_mode == Mode::Plan ? Mode::Auto : Mode::Plan);
        return true;
      }
    }
    // Press 't' to toggle thinking visibility
    if (event.is_character() && event.character() == "t") {
      if (state_.user_input.empty()) {
        state_.show_thinking = !state_.show_thinking;
        return true;
      }
    }
    // Press 'y' for accept when complete
    if (event.is_character() && event.character() == "y" &&
        state_.current_stage == PipelineStage::Complete &&
        state_.user_input.empty()) {
      if (on_accept_)
        on_accept_();
      state_.conversation_history.push_back("✓ Changes accepted");
      return true;
    }
    // Press 'n' for reject when complete
    if (event.is_character() && event.character() == "n" &&
        state_.current_stage == PipelineStage::Complete &&
        state_.user_input.empty()) {
      if (on_reject_)
        on_reject_();
      state_.conversation_history.push_back("✗ Changes rejected");
      return true;
    }
    return false;
  });

  return Renderer(input_with_hotkeys, [=] {
    return hbox({text("❯ ") | color(Color::GreenLight) | bold,
                 input_with_hotkeys->Render() | flex});
  });
}

std::string TUI::StageToString(PipelineStage stage) {
  switch (stage) {
  case PipelineStage::Idle:
    return "Idle";
  case PipelineStage::Planning:
    return "Planning";
  case PipelineStage::ToolExecution:
    return "Executing tools";
  case PipelineStage::CodeDrafting:
    return "Generating code";
  case PipelineStage::StyleEnforcing:
    return "Applying style";
  case PipelineStage::ComplexityAuditing:
    return "Analyzing complexity";
  case PipelineStage::GatekeeperReview:
    return "Final review";
  case PipelineStage::Complete:
    return "Complete";
  case PipelineStage::Error:
    return "Error";
  default:
    return "Unknown";
  }
}

std::string TUI::ModeToString(Mode mode) {
  return mode == Mode::Plan ? "Plan" : "Auto";
}

} // namespace ui
} // namespace zweek
