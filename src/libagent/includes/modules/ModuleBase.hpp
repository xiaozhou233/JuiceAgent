#pragma once
#include <modules/IModule.hpp>

namespace JuiceAgent::Core::Modules {

class ModuleBase : public IModule {
public:
    virtual ~ModuleBase() = default;

    bool init() override {
        if (_initialized) {
            return true;
        }

        if (!on_init()) {
            return false;
        }

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

        if (!on_start()) {
            return false;
        }

        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        on_stop();
        _running = false;
    }

    bool initialized() const override {
        return _initialized;
    }

    bool running() const override {
        return _running;
    }

protected:
    virtual bool on_init() {
        return true;
    }

    virtual bool on_start() {
        return true;
    }

    virtual void on_stop() {
    }

private:
    bool _initialized = false;
    bool _running = false;
};

}