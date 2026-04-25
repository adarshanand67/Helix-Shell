#ifndef HELIX_ENVIRONMENT_EXPANDER_H
#define HELIX_ENVIRONMENT_EXPANDER_H

#include "executor/interfaces.h"
#include <string>
#include <pwd.h>

namespace helix {

struct ShellState;

class EnvironmentVariableExpander : public IEnvironmentExpander {
public:
    EnvironmentVariableExpander() = default;
    ~EnvironmentVariableExpander() override = default;

    std::string expand(const std::string& input) const override;
    std::string expandWithState(const std::string& input, const ShellState* state) const;

private:
    std::string getVariableValue(const std::string& name) const;
    std::string getVariableValueWithState(const std::string& name, const ShellState* state) const;
};

} // namespace helix

#endif // HELIX_ENVIRONMENT_EXPANDER_H
