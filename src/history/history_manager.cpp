#define NOMINMAX
#include "history/history_manager.hpp"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <shlobj.h>
#endif

using json = nlohmann::json;

namespace zweek {
namespace history {

HistoryManager::HistoryManager() {
  // Constructor
}

HistoryManager::~HistoryManager() {
  // Destructor
}

bool HistoryManager::Init(const std::string& db_path) {
  // For in-memory version, just mark as initialized
  initialized_ = true;
  current_session_id_ = GenerateSessionId();
  return true;
}

void HistoryManager::Close() {
  // No-op for in-memory version
}

std::string HistoryManager::GenerateSessionId() {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::system_clock::to_time_t(now);
  
  std::stringstream ss;
  ss << "session_" << timestamp;
  return ss.str();
}

void HistoryManager::LogOperation(const std::string& operation_type, const std::string& details) {
  if (!initialized_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::system_clock::to_time_t(now);

  Operation op;
  op.id = next_operation_id_++;
  op.timestamp = timestamp;
  op.operation_type = operation_type;
  op.details = details;
  op.session_id = current_session_id_;

  operations_.push_back(op);
}

void HistoryManager::SnapshotFile(const std::string& file_path, const std::string& content) {
  if (!initialized_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::system_clock::to_time_t(now);

  FileSnapshot snapshot;
  snapshot.id = next_snapshot_id_++;
  snapshot.timestamp = timestamp;
  snapshot.file_path = file_path;
  snapshot.content = content;
  snapshot.operation_id = next_operation_id_ - 1; // Last operation

  snapshots_.push_back(snapshot);
}

std::string HistoryManager::RestoreFile(const std::string& file_path, int version) {
  if (!initialized_) return "";

  std::lock_guard<std::mutex> lock(mutex_);

  // Find snapshots for this file
  std::vector<FileSnapshot> file_snapshots;
  for (const auto& snapshot : snapshots_) {
    if (snapshot.file_path == file_path) {
      file_snapshots.push_back(snapshot);
    }
  }

  if (file_snapshots.empty()) {
    return "";
  }

  // Sort by timestamp
  std::sort(file_snapshots.begin(), file_snapshots.end(),
            [](const FileSnapshot& a, const FileSnapshot& b) {
              return a.timestamp < b.timestamp;
            });

  if (version < 0) {
    // Return most recent
    return file_snapshots.back().content;
  } else if (version < static_cast<int>(file_snapshots.size())) {
    return file_snapshots[version].content;
  }

  return "";
}

std::vector<FileSnapshot> HistoryManager::GetFileHistory(const std::string& file_path) {
  std::vector<FileSnapshot> result;
  if (!initialized_) return result;

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& snapshot : snapshots_) {
    if (snapshot.file_path == file_path) {
      result.push_back(snapshot);
    }
  }

  // Sort by timestamp descending
  std::sort(result.begin(), result.end(),
            [](const FileSnapshot& a, const FileSnapshot& b) {
              return a.timestamp > b.timestamp;
            });

  return result;
}

std::vector<Operation> HistoryManager::GetRecentOperations(int limit) {
  std::vector<Operation> result;
  if (!initialized_) return result;

  std::lock_guard<std::mutex> lock(mutex_);

  // Get last N operations
  int start = (std::max)(0, static_cast<int>(operations_.size()) - limit);
  for (int i = static_cast<int>(operations_.size()) - 1; i >= start; --i) {
    result.push_back(operations_[i]);
  }

  return result;
}

std::vector<Operation> HistoryManager::GetOperationsByType(const std::string& operation_type, int limit) {
  std::vector<Operation> result;
  if (!initialized_) return result;

  std::lock_guard<std::mutex> lock(mutex_);

  // Filter by type and get last N
  for (auto it = operations_.rbegin(); it != operations_.rend() && result.size() < static_cast<size_t>(limit); ++it) {
    if (it->operation_type == operation_type) {
      result.push_back(*it);
    }
  }

  return result;
}

void HistoryManager::CreateSessionSnapshot(const std::string& name, const std::string& description) {
  if (!initialized_) return;

  // For in-memory version, just log the snapshot creation
  LogOperation("session_snapshot", "Created snapshot: " + name + " - " + description);
}

void HistoryManager::StartNewSession() {
  std::lock_guard<std::mutex> lock(mutex_);
  current_session_id_ = GenerateSessionId();
}

void HistoryManager::LogChatMessage(const std::string& role, const std::string& content) {
  if (!initialized_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::system_clock::to_time_t(now);

  ChatMessage msg;
  msg.id = next_chat_message_id_++;
  msg.timestamp = timestamp;
  msg.role = role;
  msg.content = content;
  msg.session_id = current_session_id_;

  chat_messages_.push_back(msg);
  
  // Prune if we exceed max
  if (chat_messages_.size() > MAX_CHAT_MESSAGES) {
    chat_messages_.erase(chat_messages_.begin(), 
                         chat_messages_.begin() + (chat_messages_.size() - MAX_CHAT_MESSAGES));
  }
}

std::vector<ChatMessage> HistoryManager::GetChatHistory(int limit) {
  std::vector<ChatMessage> result;
  if (!initialized_) return result;

  std::lock_guard<std::mutex> lock(mutex_);

  if (limit < 0 || limit > static_cast<int>(chat_messages_.size())) {
    return chat_messages_;
  }

  int start = static_cast<int>(chat_messages_.size()) - limit;
  for (int i = start; i < static_cast<int>(chat_messages_.size()); ++i) {
    result.push_back(chat_messages_[i]);
  }

  return result;
}

std::vector<ChatMessage> HistoryManager::GetChatHistoryBySession(const std::string& session_id, int limit) {
  std::vector<ChatMessage> result;
  if (!initialized_) return result;

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& msg : chat_messages_) {
    if (msg.session_id == session_id) {
      result.push_back(msg);
      if (limit > 0 && static_cast<int>(result.size()) >= limit) {
        break;
      }
    }
  }

  return result;
}

void HistoryManager::ClearChatHistory() {
  if (!initialized_) return;
  
  std::lock_guard<std::mutex> lock(mutex_);
  chat_messages_.clear();
}

std::string HistoryManager::SerializeToJson() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  json j;
  j["version"] = 1;
  j["session_id"] = current_session_id_;
  j["timestamp"] = std::time(nullptr);
  
  // Serialize chat messages
  json chat_array = json::array();
  for (const auto& msg : chat_messages_) {
    json msg_obj;
    msg_obj["id"] = msg.id;
    msg_obj["timestamp"] = static_cast<long long>(msg.timestamp);
    msg_obj["role"] = msg.role;
    msg_obj["content"] = msg.content;
    msg_obj["session_id"] = msg.session_id;
    chat_array.push_back(msg_obj);
  }
  j["chat_messages"] = chat_array;
  
  // Serialize operations
  json ops_array = json::array();
  for (const auto& op : operations_) {
    json op_obj;
    op_obj["id"] = op.id;
    op_obj["timestamp"] = static_cast<long long>(op.timestamp);
    op_obj["operation_type"] = op.operation_type;
    op_obj["details"] = op.details;
    op_obj["session_id"] = op.session_id;
    ops_array.push_back(op_obj);
  }
  j["operations"] = ops_array;
  
  return j.dump(2);
}

bool HistoryManager::DeserializeFromJson(const std::string& json_data) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json j = json::parse(json_data);
    
    // Check version
    int version = j.value("version", 0);
    if (version != 1) {
      return false; // Unsupported version
    }
    
    // Load session ID
    current_session_id_ = j.value("session_id", GenerateSessionId());
    
    // Load chat messages
    if (j.contains("chat_messages")) {
      chat_messages_.clear();
      for (const auto& msg_obj : j["chat_messages"]) {
        ChatMessage msg;
        msg.id = msg_obj.value("id", 0);
        msg.timestamp = static_cast<std::time_t>(msg_obj.value("timestamp", 0LL));
        msg.role = msg_obj.value("role", "");
        msg.content = msg_obj.value("content", "");
        msg.session_id = msg_obj.value("session_id", "");
        chat_messages_.push_back(msg);
        
        // Update ID counter
        if (msg.id >= next_chat_message_id_) {
          next_chat_message_id_ = msg.id + 1;
        }
      }
    }
    
    // Load operations
    if (j.contains("operations")) {
      operations_.clear();
      for (const auto& op_obj : j["operations"]) {
        Operation op;
        op.id = op_obj.value("id", 0);
        op.timestamp = static_cast<std::time_t>(op_obj.value("timestamp", 0LL));
        op.operation_type = op_obj.value("operation_type", "");
        op.details = op_obj.value("details", "");
        op.session_id = op_obj.value("session_id", "");
        operations_.push_back(op);
        
        // Update ID counter
        if (op.id >= next_operation_id_) {
          next_operation_id_ = op.id + 1;
        }
      }
    }
    
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool HistoryManager::SaveToFile(const std::string& file_path) {
  try {
    std::string json_data = SerializeToJson();
    
    // Create directory if not exists
    std::filesystem::path path(file_path);
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }

    // Atomic write: write to temp file, then rename
    std::string temp_path = file_path + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
      return false;
    }
    
    out << json_data;
    out.close();
    
    // Rename temp to actual file (atomic on most filesystems)
    #ifdef _WIN32
      // Windows: need to remove target first
      std::remove(file_path.c_str());
    #endif
    
    if (std::rename(temp_path.c_str(), file_path.c_str()) != 0) {
      std::remove(temp_path.c_str()); // Clean up temp file
      return false;
    }
    
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool HistoryManager::LoadFromFile(const std::string& file_path) {
  try {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
      return false;
    }
    
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string json_data = buffer.str();
    
    return DeserializeFromJson(json_data);
  } catch (const std::exception&) {
    return false;
  }
}

#ifdef _WIN32
  std::string HistoryManager::GetSessionsDirectory() const {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
      return std::string(path) + "\\.zweek\\sessions";
    }
    return "."; // Fallback
  }
#else
  std::string HistoryManager::GetSessionsDirectory() const {
    const char* home = getenv("HOME");
    if (home) {
      return std::string(home) + "/.zweek/sessions";
    }
    return "."; // Fallback
  }
#endif

  std::string HistoryManager::GetDefaultHistoryPath() const {
    std::string base_path = GetSessionsDirectory();
    #ifdef _WIN32
      return base_path + "\\" + current_session_id_ + ".json";
    #else
      return base_path + "/" + current_session_id_ + ".json";
    #endif
  }

  std::vector<std::string> HistoryManager::GetAvailableSessions() const {
    std::vector<std::string> sessions;
    std::string dir_path = GetSessionsDirectory();
    
    if (!std::filesystem::exists(dir_path)) {
      return sessions;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        std::string filename = entry.path().stem().string();
        sessions.push_back(filename);
      }
    }
    
    // Sort descending (newest first)
    std::sort(sessions.rbegin(), sessions.rend());
    
    return sessions;
  }

} // namespace history
} // namespace zweek

