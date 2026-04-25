#include "ai_provider.h"
#include <array>
#include <cstdlib>
#include <memory>

namespace helix {

static std::string runCurl(const std::string& cmd) {
    std::array<char, 256> buf;
    std::string out;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get())) out += buf.data();
    return out;
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    size_t end = pos;
    while (end < json.size()) {
        if (json[end] == '"' && (end == 0 || json[end-1] != '\\')) break;
        ++end;
    }
    std::string raw = json.substr(pos, end - pos);
    std::string clean;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i+1 < raw.size()) {
            switch (raw[i+1]) {
                case 'n':  clean += '\n'; ++i; break;
                case 't':  clean += '\t'; ++i; break;
                case '"':  clean += '"';  ++i; break;
                case '\\': clean += '\\'; ++i; break;
                default:   clean += raw[i]; break;
            }
        } else {
            clean += raw[i];
        }
    }
    return clean;
}

AiProvider detectAiProvider() {
    AiProvider p;

    const char* provider_env = getenv("HELIX_AI_PROVIDER");
    std::string provider = provider_env ? provider_env : "";

    if (provider.empty()) {
        if      (getenv("ANTHROPIC_API_KEY"))                          provider = "anthropic";
        else if (getenv("OPENAI_API_KEY"))                             provider = "openai";
        else if (getenv("GOOGLE_API_KEY") || getenv("GEMINI_API_KEY")) provider = "google";
        else if (getenv("GROQ_API_KEY"))                               provider = "groq";
        else                                                            provider = "ollama";
    }

    const char* model_env = getenv("HELIX_AI_MODEL");

    if (provider == "anthropic") {
        p.name    = "anthropic";
        p.model   = model_env ? model_env : "claude-haiku-4-5-20251001";
        const char* k = getenv("ANTHROPIC_API_KEY");
        p.api_key = k ? k : "";
        p.base_url = "https://api.anthropic.com/v1/messages";

    } else if (provider == "openai") {
        p.name    = "openai";
        p.model   = model_env ? model_env : "gpt-4o-mini";
        const char* k = getenv("OPENAI_API_KEY");
        p.api_key = k ? k : "";
        p.base_url = "https://api.openai.com/v1/chat/completions";

    } else if (provider == "google") {
        p.name    = "google";
        p.model   = model_env ? model_env : "gemini-2.0-flash";
        const char* k = getenv("GOOGLE_API_KEY");
        if (!k) k = getenv("GEMINI_API_KEY");
        p.api_key = k ? k : "";
        p.base_url = "https://generativelanguage.googleapis.com/v1beta/models/"
                     + p.model + ":generateContent?key=" + p.api_key;

    } else if (provider == "groq") {
        p.name    = "groq";
        p.model   = model_env ? model_env : "llama3-8b-8192";
        const char* k = getenv("GROQ_API_KEY");
        p.api_key = k ? k : "";
        p.base_url = "https://api.groq.com/openai/v1/chat/completions";

    } else {
        p.name    = "ollama";
        p.model   = model_env ? model_env : "llama3";
        p.base_url = "http://localhost:11434/api/chat";
    }

    return p;
}

std::string callAiProvider(const AiProvider& p,
                             const std::string& system_prompt,
                             const std::string& user_msg,
                             int max_tokens) {
    std::string body, curl_cmd, response;

    if (p.name == "anthropic") {
        body = "{\"model\":\"" + p.model + "\","
               "\"max_tokens\":" + std::to_string(max_tokens) + ","
               "\"system\":\"" + jsonEscape(system_prompt) + "\","
               "\"messages\":[{\"role\":\"user\",\"content\":\""
               + jsonEscape(user_msg) + "\"}]}";
        curl_cmd = "curl -s " + p.base_url
                 + " -H 'content-type: application/json'"
                   " -H 'anthropic-version: 2023-06-01'"
                   " -H 'x-api-key: " + p.api_key + "'"
                   " -d '" + body + "'";
        response = runCurl(curl_cmd);
        return extractJsonString(response, "text");

    } else if (p.name == "openai" || p.name == "groq") {
        body = "{\"model\":\"" + p.model + "\","
               "\"max_tokens\":" + std::to_string(max_tokens) + ","
               "\"messages\":["
               "{\"role\":\"system\",\"content\":\"" + jsonEscape(system_prompt) + "\"},"
               "{\"role\":\"user\",\"content\":\"" + jsonEscape(user_msg) + "\"}]}";
        curl_cmd = "curl -s " + p.base_url
                 + " -H 'content-type: application/json'"
                   " -H 'Authorization: Bearer " + p.api_key + "'"
                   " -d '" + body + "'";
        response = runCurl(curl_cmd);
        return extractJsonString(response, "content");

    } else if (p.name == "google") {
        body = "{\"contents\":[{\"parts\":[{\"text\":\""
               + jsonEscape(system_prompt + "\n" + user_msg) + "\"}]}],"
               "\"generationConfig\":{\"maxOutputTokens\":" + std::to_string(max_tokens) + "}}";
        curl_cmd = "curl -s " + p.base_url
                 + " -H 'content-type: application/json'"
                   " -d '" + body + "'";
        response = runCurl(curl_cmd);
        return extractJsonString(response, "text");

    } else {
        // ollama — local, free
        body = "{\"model\":\"" + p.model + "\","
               "\"stream\":false,"
               "\"messages\":["
               "{\"role\":\"system\",\"content\":\"" + jsonEscape(system_prompt) + "\"},"
               "{\"role\":\"user\",\"content\":\"" + jsonEscape(user_msg) + "\"}]}";
        curl_cmd = "curl -s " + p.base_url
                 + " -H 'content-type: application/json'"
                   " -d '" + body + "'";
        response = runCurl(curl_cmd);
        return extractJsonString(response, "content");
    }
}

} // namespace helix
