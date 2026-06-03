/*
 * IpswdClient.cpp
 */

#include "IpswdClient.h"

#include "ToolRunner.h"

#include <cstdlib>
#include <sstream>
#include <utility>
#include <vector>

namespace PP {

namespace {

std::string resolveBaseUrl(const std::string& baseUrl) {
    if (!baseUrl.empty()) {
        return baseUrl;
    }
    return IpswdClient::defaultBaseUrl();
}

std::string stripTrailingSlash(std::string url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

std::string ensureV1Base(std::string url) {
    url = stripTrailingSlash(url);
    if (url.size() >= 3 && url.compare(url.size() - 3, 3, "/v1") == 0) {
        return url;
    }
    return url + "/v1";
}

IpswdResponse curlGet(const std::string& url) {
    IpswdResponse response;
    const std::string curl = ToolRunner::findExecutable("curl");
    if (curl.empty()) {
        response.error = "curl not found";
        return response;
    }

    std::vector<std::string> argv;
    argv.push_back(curl);
    argv.push_back("-sS");
    argv.push_back("--connect-timeout");
    argv.push_back("2");
    argv.push_back("--max-time");
    argv.push_back("120");
    argv.push_back("-w");
    argv.push_back("\n%{http_code}");
    argv.push_back(url);

    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0) {
        response.error = run.stderrText.empty() ? "curl request failed" : run.stderrText;
        return response;
    }

    const std::string& out = run.stdoutText;
    const size_t lastNl = out.find_last_of('\n');
    if (lastNl == std::string::npos || lastNl + 1 >= out.size()) {
        response.body = out;
        response.httpStatus = 200;
        return response;
    }

    const std::string codeStr = out.substr(lastNl + 1);
    response.body = out.substr(0, lastNl);
    try {
        response.httpStatus = std::stoi(codeStr);
    } catch (...) {
        response.httpStatus = 0;
        response.error = "invalid HTTP status from curl";
    }
    return response;
}

IpswdResponse curlGetEncoded(const std::string& endpoint,
                             const std::string& baseUrl,
                             const std::vector<std::pair<std::string, std::string>>& params) {
    const std::string curl = ToolRunner::findExecutable("curl");
    if (curl.empty()) {
        IpswdResponse response;
        response.error = "curl not found";
        return response;
    }

    std::vector<std::string> argv;
    argv.push_back(curl);
    argv.push_back("-sS");
    argv.push_back("--connect-timeout");
    argv.push_back("2");
    argv.push_back("--max-time");
    argv.push_back("120");
    argv.push_back("-G");
    argv.push_back(ensureV1Base(baseUrl) + endpoint);
    for (size_t i = 0; i < params.size(); ++i) {
        argv.push_back("--data-urlencode");
        argv.push_back(params[i].first + "=" + params[i].second);
    }
    argv.push_back("-w");
    argv.push_back("\n%{http_code}");

    const CommandResult run = ToolRunner::run(argv);
    IpswdResponse response;
    if (run.exitCode != 0) {
        response.error = run.stderrText.empty() ? "curl request failed" : run.stderrText;
        return response;
    }

    const std::string& out = run.stdoutText;
    const size_t lastNl = out.find_last_of('\n');
    if (lastNl == std::string::npos || lastNl + 1 >= out.size()) {
        response.body = out;
        response.httpStatus = 200;
        return response;
    }

    const std::string codeStr = out.substr(lastNl + 1);
    response.body = out.substr(0, lastNl);
    try {
        response.httpStatus = std::stoi(codeStr);
    } catch (...) {
        response.httpStatus = 0;
        response.error = "invalid HTTP status from curl";
    }
    return response;
}

} /* anonymous */

std::string IpswdClient::defaultBaseUrl() {
    const char* env = std::getenv("PURPLEPOIS0N_IPSWD");
    if (env != nullptr && env[0] != '\0') {
        return ensureV1Base(env);
    }
    return "http://127.0.0.1:3993/v1";
}

bool IpswdClient::ping(const std::string& baseUrl) {
    const IpswdResponse response = curlGet(ensureV1Base(resolveBaseUrl(baseUrl)) + "/_ping");
    return response.ok();
}

IpswdResponse IpswdClient::getMachoInfo(const std::string& machoPath,
                                        const std::string& arch,
                                        const std::string& baseUrl) {
    std::vector<std::pair<std::string, std::string>> params;
    params.push_back(std::make_pair("path", machoPath));
    if (!arch.empty()) {
        params.push_back(std::make_pair("arch", arch));
    }
    return curlGetEncoded("/macho/info", resolveBaseUrl(baseUrl), params);
}

IpswdResponse IpswdClient::getDscInfo(const std::string& dscPath, const std::string& baseUrl) {
    std::vector<std::pair<std::string, std::string>> params;
    params.push_back(std::make_pair("path", dscPath));
    return curlGetEncoded("/dsc/info", resolveBaseUrl(baseUrl), params);
}

} /* namespace PP */
