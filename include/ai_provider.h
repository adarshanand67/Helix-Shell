#ifndef HELIX_AI_PROVIDER_H
#define HELIX_AI_PROVIDER_H

#include <string>

namespace helix {

struct AiProvider {
    std::string name;     // anthropic | openai | google | groq | ollama
    std::string model;
    std::string api_key;
    std::string base_url;
};

// Detect provider from environment. Priority:
//   HELIX_AI_PROVIDER (explicit) > first set key (auto) > ollama (local fallback)
// Override model with HELIX_AI_MODEL.
AiProvider detectAiProvider();

// Call the provider and return the plain-text response.
// Returns empty string on failure.
std::string callAiProvider(const AiProvider& provider,
                            const std::string& system_prompt,
                            const std::string& user_message,
                            int max_tokens = 512);

} // namespace helix

#endif // HELIX_AI_PROVIDER_H
