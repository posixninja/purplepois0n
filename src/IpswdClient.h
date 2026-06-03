/*
 * IpswdClient.h — HTTP client for blacktop ipswd (REST API on :3993).
 */

#ifndef IPSWD_CLIENT_H_
#define IPSWD_CLIENT_H_

#include <string>

namespace PP {

/** Result of an ipswd HTTP call. */
struct IpswdResponse {
    int httpStatus = 0;
    std::string body;
    std::string error;
    bool ok() const { return httpStatus >= 200 && httpStatus < 300 && error.empty(); }
};

class IpswdClient {
public:
    /** Base URL from PURPLEPOIS0N_IPSWD or http://127.0.0.1:3993/v1 */
    static std::string defaultBaseUrl();

    /** GET /v1/_ping — daemon reachable. */
    static bool ping(const std::string& baseUrl = std::string());

    static IpswdResponse getMachoInfo(const std::string& machoPath,
                                      const std::string& arch = std::string(),
                                      const std::string& baseUrl = std::string());

    static IpswdResponse getDscInfo(const std::string& dscPath,
                                    const std::string& baseUrl = std::string());
};

} /* namespace PP */

#endif /* IPSWD_CLIENT_H_ */
