#pragma once

#include <jni.h>
#include <jvmti.h>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace JuiceAgent {
namespace Services {

struct ClassFileData {
    std::string classname;
    std::vector<unsigned char> bytecode;
};

struct PendingPatch {
    std::vector<unsigned char> bytecode;
};

class BytecodeStore {
public:
    static BytecodeStore& getInstance() {
        static BytecodeStore instance;
        return instance;
    }

    BytecodeStore(const BytecodeStore&) = delete;
    BytecodeStore& operator=(const BytecodeStore&) = delete;

    // ---- Capture API (called by JNI or other services) ----

    // Mark a class (internal name, e.g. "java/lang/String") for capture
    // on the next ClassFileLoadHook.
    void requestCapture(std::string classname) {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingCaptures_.insert(std::move(classname));
    }

    // Returns the captured bytecode if available, removing it from the cache.
    // Returns std::nullopt if not yet captured.
    std::optional<ClassFileData> takeCaptured(const std::string& classname) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = captured_.find(classname);
        if (it == captured_.end()) {
            return std::nullopt;
        }
        ClassFileData data = std::move(it->second);
        captured_.erase(it);
        return data;
    }

    // Peek without removing.
    std::optional<ClassFileData> peekCaptured(const std::string& classname) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = captured_.find(classname);
        if (it == captured_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // ---- Patch API (called by JNI or other services) ----

    // Queue a retransform: the next ClassFileLoadHook for this class will
    // replace its bytecode with the provided buffer.
    void requestPatch(std::string classname, std::vector<unsigned char> bytecode) {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingPatches_.insert_or_assign(
            std::move(classname),
            PendingPatch{std::move(bytecode)}
        );
    }

private:
    friend class BytecodeService;

    BytecodeStore() = default;

    // Called by BytecodeService::captureBytecodes under internal lock.
    // Stores the raw bytes if this class is pending capture.
    // Returns the captured byte length, or 0 if no capture was pending.
    std::size_t tryCapture(const char* name, const unsigned char* bytes, jint len) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!name || !bytes || len <= 0) return 0;

        auto reqIt = pendingCaptures_.find(name);
        if (reqIt == pendingCaptures_.end()) return 0;
        if (captured_.find(name) != captured_.end()) {
            pendingCaptures_.erase(reqIt);
            return 0;
        }

        ClassFileData data;
        data.classname = name;
        data.bytecode.assign(bytes, bytes + len);

        std::size_t capturedLen = data.bytecode.size();
        captured_.emplace(name, std::move(data));
        pendingCaptures_.erase(reqIt);
        return capturedLen;
    }

    // Called by BytecodeService::patchBytecodes under internal lock.
    // If a pending patch exists for `name`, moves its bytecode out and
    // returns true; otherwise returns false.
    bool takePatch(const char* name, std::vector<unsigned char>& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!name) return false;
        auto it = pendingPatches_.find(name);
        if (it == pendingPatches_.end()) return false;
        out = std::move(it->second.bytecode);
        pendingPatches_.erase(it);
        return true;
    }

    // Put a failed patch back so it can be retried on the next hook.
    void putBackPatch(const char* name, std::vector<unsigned char>&& bytes) {
        if (!name || bytes.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        pendingPatches_.insert_or_assign(
            name, PendingPatch{std::move(bytes)});
    }

    mutable std::mutex mutex_;
    std::unordered_set<std::string> pendingCaptures_;
    std::unordered_map<std::string, ClassFileData> captured_;
    std::unordered_map<std::string, PendingPatch> pendingPatches_;
};

}
}
