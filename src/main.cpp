#include "pipeline/orchestrator.hpp"
#include "ui/tui.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace zweek::ui;
using namespace zweek::pipeline;

int main(int argc, char **argv) {
  TUI tui;
  Orchestrator orchestrator;

  // Connect orchestrator callbacks to TUI
  orchestrator.SetProgressCallback(
      [&](const std::string &status) { tui.AddToHistory(status); });

  orchestrator.SetResponseCallback([&](const std::string &response) {
    // Only add to history if response is not empty
    if (!response.empty()) {
      tui.AddToHistory(response);
    }
    tui.UpdateStage(PipelineStage::Complete, 1.0f);
  });

  orchestrator.SetStreamCallback([&](const std::string &chunk) {
    tui.AppendToLastMessage(chunk);
  });
  
  // Connect interrupt flag from TUI to orchestrator
  orchestrator.SetInterruptFlag(&tui.GetState().interrupt_inference_);

  // Set up TUI callbacks
  tui.SetOnSubmit([&](const std::string &request) {
    std::cout << "Processing: " << request << std::endl;
    
    // Reset interrupt flag before starting new request
    tui.GetState().interrupt_inference_.store(false);

    // Run orchestrator in background thread
    std::thread pipeline_thread([&, request]() {
      tui.UpdateStage(PipelineStage::Planning, 0.1f);
      orchestrator.ProcessRequest(request);
    });
    pipeline_thread.detach();
  });

  tui.SetOnAccept([]() { std::cout << "Changes accepted!" << std::endl; });

  tui.SetOnReject([]() { std::cout << "Changes rejected!" << std::endl; });

  tui.SetOnModify(
      []() { std::cout << "Requesting modifications..." << std::endl; });
  
  // Spinner animation thread - update every 100ms
  std::atomic<bool> running{true};
  std::thread spinner_thread([&]() {
    while (running) {
      tui.GetState().spinner_frame++;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
  spinner_thread.detach();

  // Run the TUI
  tui.Run();
  
  running = false; // Stop spinner thread

  return 0;
}
