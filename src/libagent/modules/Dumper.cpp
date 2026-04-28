#include <modules/ModuleRegistry.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <event/eventbus.hpp>
#include <event/event_type.hpp>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace JuiceAgent::Core::Modules {

namespace fs = std::filesystem;

struct DumperConfig {
    bool enabled = false;
    std::string DumpDir;
};

static DumperConfig config;
static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

class Dumper : public IModule {
public:
    std::string name() const override {
        return "Dumper";
    }

    bool init() override {
        if (_initialized) {
            return true;
        }

        auto& cfg = agent.get_config();

        config.enabled = cfg.get<bool>(
            "JuiceAgent.Modules.Dumper.Enabled",
            false
        );

        config.DumpDir = cfg.get<std::string>(
            "JuiceAgent.Modules.Dumper.DumpDir",
            "./dump",
            true
        );

        _initialized = true;
        return true;
    }

    bool start() override {
        if (!_initialized) {
            return false;
        }

        if (_running) {
            return true;
        }

        if (!config.enabled) {
            return true;
        }

        auto& bus = agent.get_eventbus();

        _classFileToken = bus.subscribe<EventClassFileLoadHook>(
            [this](const auto& e) {
                on_class_file_load_hook(e);
            }
        );

        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        auto& bus = agent.get_eventbus();
        bus.unsubscribe<EventClassFileLoadHook>(_classFileToken);

        _running = false;
    }

    void on_class_file_load_hook(const EventClassFileLoadHook& event) {
        if (!event.classbytes || event.class_data_len <= 0) {
            return;
        }

        std::string_view name = event.name ? event.name : "";
        if (name.empty()) {
            return;
        }

        try {
            fs::path output = build_class_path(name);

            {
                std::lock_guard<std::mutex> lock(_mutex);
                fs::create_directories(output.parent_path());
            }

            std::ofstream file(output, std::ios::binary);
            if (!file.is_open()) {
                PLOGW << "Dumper: Failed to open file: " << output.string();
                return;
            }

            file.write(
                reinterpret_cast<const char*>(event.classbytes),
                static_cast<std::streamsize>(event.class_data_len)
            );

            file.close();
        } catch (const std::exception& e) {
            PLOGE << "Dumper: " << e.what();
        }
    }

private:
    fs::path build_class_path(std::string_view className) {
        fs::path path(config.DumpDir);

        // JVM name format: cn/xiaozhou233/Test
        path /= fs::path(className);
        path += ".class";

        return path;
    }

private:
    bool _initialized = false;
    bool _running = false;

    EventBus::Token _classFileToken = 0;
    std::mutex _mutex;
};

}

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::Dumper,
    Dumper
)