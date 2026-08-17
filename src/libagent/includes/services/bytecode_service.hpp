#pragma once

#include <services/base.hpp>
#include <event/event_type.hpp>

namespace JuiceAgent {
namespace Services {

class BytecodeService : public IService {
public:
    static BytecodeService& getInstance();

    const char* name() const override { return "BytecodeService"; }

    bool onInitialize() override;

    void onShutdown() override;

private:
    void onClassFileLoad(const EventClassFileLoadHook& event);
};

}
}
