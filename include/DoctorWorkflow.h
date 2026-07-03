/*
 * DoctorWorkflow.h
 *
 * One-button doctors flow: detect → syringe identify → era-appropriate jailbreak probe/execute.
 */

#ifndef DOCTOR_WORKFLOW_H_
#define DOCTOR_WORKFLOW_H_

#include "Gen0Workflow.h"

namespace PP {

class DeviceManager;

/** Run the doctors orchestration path with JSON step events on stdout. */
bool runDoctorFlow(DeviceManager& manager,
                   const std::string& targetUDID,
                   const Gen0Options& options);

} /* namespace PP */

#endif /* DOCTOR_WORKFLOW_H_ */
