/*
 * DoctorReporter.h
 *
 * JSON-line step protocol for doctors GUI (--doctor-run).
 * Each host↔device step emits explicit request/response events on stdout.
 */

#ifndef DOCTOR_REPORTER_H_
#define DOCTOR_REPORTER_H_

#include <cstdint>
#include <string>

namespace PP {

class DoctorReporter {
public:
    static DoctorReporter& instance();

    void setEnabled(bool enabled);
    bool enabled() const { return mEnabled; }

    void stepRequest(const std::string& stepId, const std::string& detail);
    void stepResponse(const std::string& stepId,
                      bool success,
                      const std::string& detail,
                      uint32_t status = 0);

    void syringeRequest(const std::string& transport, const std::string& command);
    void syringeResponse(const std::string& transport,
                         bool success,
                         uint32_t status,
                         const std::string& detail);

    void complete(bool success, const std::string& detail = std::string());

private:
    DoctorReporter() = default;
    void emitLine(const std::string& json);

    bool mEnabled = false;
};

} /* namespace PP */

#endif /* DOCTOR_REPORTER_H_ */
