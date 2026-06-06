/*
 * InstproxyService.h
 *
 * installation_proxy client for IPA install (research / sideload).
 */

#ifndef INSTPROXY_SERVICE_H_
#define INSTPROXY_SERVICE_H_

#include <string>

namespace PP {

class InstproxyService {
public:
    explicit InstproxyService(const std::string& udid);

    /** Connect instproxy and disconnect (probe-only). */
    bool probe(std::string* errorMessage);

    /** Install IPA at @p ipaPath; blocks until complete or error. */
    bool install(const std::string& ipaPath, std::string* errorMessage);

private:
    std::string mUdid;
};

} /* namespace PP */

#endif /* INSTPROXY_SERVICE_H_ */
