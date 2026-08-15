#pragma once

namespace JuiceAgent::Loader {

// Attach to the JVM and initialize JuiceAgent.
// runtime_dir: directory containing config.toml, or nullptr to use defaults.
void entrypoint(const char* runtime_dir);

} // namespace JuiceAgent::Loader
