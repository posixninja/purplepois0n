/*
 * ChainRunner.h
 *
 * Staged probe/execute orchestration for registered primitives.
 */

#ifndef PRIMITIVES_CHAIN_RUNNER_H_
#define PRIMITIVES_CHAIN_RUNNER_H_

#include "PrimitiveTypes.h"

namespace PP {
namespace primitives {

struct ChainReport {
    ChainStage stage = ChainStage::Detect;
    PrimitiveResult result = PrimitiveResult::Success;
    std::string message;
};

class ChainRunner {
public:
    ChainRunner();

    /** Detect → Connect → Probe → Report (no mutation). */
    bool runProbeChain(ExecutionContext& context);

    /** Execute mutating primitives when allowMutation is set. */
    bool runExecuteChain(ExecutionContext& context);

    const std::vector<ChainReport>& reports() const { return mReports; }

    /** Write accumulated reports as JSON to @p path. */
    bool writeReportToFile(const std::string& path) const;

private:
    void logStage(ChainStage stage, const std::string& message);
    void recordReport(ChainStage stage, PrimitiveResult result, const std::string& message);
    bool runStage(ChainStage stage, ExecutionContext& context, bool executeMode);

    std::vector<ChainReport> mReports;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_CHAIN_RUNNER_H_ */
