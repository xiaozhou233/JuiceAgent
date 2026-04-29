#pragma once
#include <string>

namespace JuiceAgent::Core::Modules {

class IModule {
public:
    virtual ~IModule() = default;

    virtual std::string name() const = 0;

    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool initialized() const = 0;
    virtual bool running() const = 0;
};

}