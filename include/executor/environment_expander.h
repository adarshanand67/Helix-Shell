#ifndef HELIX_ENVIRONMENT_EXPANDER_H
#define HELIX_ENVIRONMENT_EXPANDER_H

#include "executor/interfaces.h"
#include <string>

namespace helix {

// EnvironmentVariableExpander - Handles environment variable expansion
// Implements IEnvironmentExpander interface (Dependency Inversion Principle)
// Responsibilities:
// - Expand $VAR and ${VAR} syntax in strings
// - Handle missing environment variables gracefully
class EnvironmentVariableExpander : public IEnvironmentExpander {
public:
    EnvironmentVariableExpander() = default;
    ~EnvironmentVariableExpander() override = default;

    // Expand environment variables in a string
    // Supports both $VAR and ${VAR} syntax
    std::string expand(const std::string& input) const override;

private:
    // Get value of an environment variable
    std::string getVariableValue(const std::string& name) const;
};

} // namespace helix

#endif // HELIX_ENVIRONMENT_EXPANDER_H
