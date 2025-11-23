#include "executor/environment_expander.h"
#include <regex>
#include <cstdlib>

namespace helix {

std::string EnvironmentVariableExpander::expand(const std::string& input) const {
    std::string result = input;

    // Regular expression to match $VAR or ${VAR}
    std::regex var_regex(R"(\$\{([^}]+)\}|\$([A-Za-z_][A-Za-z0-9_]*))");

    std::smatch match;
    std::string::const_iterator search_start(input.cbegin());

    while (std::regex_search(search_start, input.cend(), match, var_regex)) {
        std::string var_name;
        if (!match[1].str().empty()) {
            // ${VAR} format
            var_name = match[1].str();
        } else if (!match[2].str().empty()) {
            // $VAR format
            var_name = match[2].str();
        }

        // Get the environment variable value
        std::string replacement = getVariableValue(var_name);

        // Replace the variable in the result
        size_t pos = result.find(match[0].str());
        if (pos != std::string::npos) {
            result.replace(pos, match[0].str().length(), replacement);
        }

        // Move search position
        search_start = match.suffix().first;
    }

    return result;
}

std::string EnvironmentVariableExpander::getVariableValue(const std::string& name) const {
    const char* var_value = getenv(name.c_str());
    return var_value ? var_value : "";
}

} // namespace helix
