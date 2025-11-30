#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <mutex>
#include <memory>

namespace zweek {
namespace history {

// Represents a logged operation
struct Operation {
  int id;
  std::time_t timestamp;
  std::string operation_type;
  std::string details;
  std::string session_id;
};

// Represents a chat message
struct ChatMessage {
  int id;
  std::time_t timestamp;
  std::string role;  // "user" or "assistant"
  std::string content;
  std::string session_id;
};

// Represents a file snapshot
struct FileSnapshot {
  int id;
  std::time_t timestamp;
  std::string file_path;
  std::string content;
  int operation_id;
};

// Manages in-memory history and file versioning
class HistoryManager {
public:
  HistoryManager();
  ~HistoryManager();

  // Initialize (for consistency with potential future SQLite version)
  bool Init(const std::string& db_path);

  // Close (no-op for in-memory version)
  void Close();

  // Operation logging
  void LogOperation(const std::string& operation_type, const std::string& details);

  // File snapshots
  void SnapshotFile(const std::string& file_path, const std::string& content);
  std::string RestoreFile(const std::string& file_path, int version = -1);
  std::vector<FileSnapshot> GetFileHistory(const std::string& file_path);

  // Query operations
  std::vector<Operation> GetRecentOperations(int limit = 50);
  std::vector<Operation> GetOperationsByType(const std::string& operation_type, int limit = 50);

  // Session management
  void CreateSessionSnapshot(const std::string& name, const std::string& description = "");
  std::string GetCurrentSessionId() const { return current_session_id_; }
  void StartNewSession();
  
  // Chat history (new)
  void LogChatMessage(const std::string& role, const std::string& content);
  std::vector<ChatMessage> GetChatHistory(int limit = -1);
  std::vector<ChatMessage> GetChatHistoryBySession(const std::string& session_id, int limit = -1);
  void ClearChatHistory();
  
  // Persistence (new)
  bool SaveToFile(const std::string& file_path);
  bool LoadFromFile(const std::string& file_path);
  std::string GetDefaultHistoryPath() const;
  std::string GetSessionsDirectory() const;
  std::vector<std::string> GetAvailableSessions() const;

  // Utility
  bool IsInitialized() const { return initialized_; }

private:
  std::string GenerateSessionId();

  // Serialization helpers
  std::string SerializeToJson();
  bool DeserializeFromJson(const std::string& json_data);
  
  // In-memory storage
  std::vector<Operation> operations_;
  std::vector<FileSnapshot> snapshots_;
  std::vector<ChatMessage> chat_messages_;
  std::mutex mutex_;
  
  bool initialized_ = false;
  std::string current_session_id_;
  int next_operation_id_ = 1;
  int next_snapshot_id_ = 1;
  int next_chat_message_id_ = 1;
  
  // Limits for memory management
  static constexpr size_t MAX_OPERATIONS = 10000;
  static constexpr size_t MAX_SNAPSHOTS = 1000;
  static constexpr size_t MAX_CHAT_MESSAGES = 10000;
};

} // namespace history
} // namespace zweek
