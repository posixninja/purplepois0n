/*
 * PrimitiveRegistry.cpp
 */

#include "primitives/PrimitiveRegistry.h"
#include "primitives/Checkm8BootromPrimitive.h"
#include "primitives/OfflinePatchPrimitive.h"
#include "primitives/IpswdHostProbePrimitive.h"
#include "primitives/SandboxCapabilityProbePrimitive.h"
#include "primitives/AfcInjectionPrimitive.h"
#include "primitives/NormalModeProbePrimitive.h"
#include "primitives/BackupProbePrimitive.h"
#include "primitives/TssSigningProbePrimitive.h"
#include "primitives/RecoveryUploadPrimitive.h"
#include "primitives/RecoveryBootChainPrimitive.h"
#include "primitives/PongoProbePrimitive.h"
#include "primitives/PongoBootChainPrimitive.h"
#include "primitives/RamdiskShellPrimitive.h"
#include "primitives/MobileBackup2ProbePrimitive.h"
#include "primitives/CodesignSigningProbePrimitive.h"
#include "primitives/SideloadPrimitive.h"
#include "primitives/HistoricalExploitModules.h"
#include "primitives/Gen6ExploitModules.h"
#include "primitives/Gen6PostExploitModules.h"
#include "primitives/RootlessBootstrapPrimitive.h"
#include "primitives/DpkgStorePrimitive.h"
#include "primitives/DeviceTreeMmioPrimitive.h"
#include "primitives/HostKernelPatchPrimitive.h"
#include "primitives/MedicinePrimitive.h"

namespace PP {
namespace primitives {

PrimitiveRegistry::PrimitiveRegistry() : mBuiltinsRegistered(false) {}

PrimitiveRegistry& PrimitiveRegistry::instance() {
    static PrimitiveRegistry registry;
    return registry;
}

void PrimitiveRegistry::registerBuiltin(std::unique_ptr<Primitive> primitive) {
    if (primitive.get() == nullptr) {
        return;
    }
    mPrimitives.push_back(std::move(primitive));
}

void PrimitiveRegistry::registerBootromPrimitives() {
    registerBuiltin(std::unique_ptr<Primitive>(new Checkm8BootromPrimitive()));
}

void PrimitiveRegistry::registerGen6ChainPrimitives() {
    registerBuiltin(std::unique_ptr<Primitive>(new KernelcacheAcquisitionModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new HostKernelPatchPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new XpfPatchfindingModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new IntegrityBypassModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new KfdExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new DarkSwordExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new WeightBufsExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new MulticastBytecopyExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new BadRecoveryPacModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new DmaFailPplModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new PageMonitorControlModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new PhysRwBuildModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new PrivilegeElevationModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new TrustCacheModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new LaunchdBootstrapModule()));
}

void PrimitiveRegistry::registerHistoricalExploitModules() {
    registerBuiltin(std::unique_ptr<Primitive>(new Limera1nExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new Kpwn24kExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new Evasi0nExploitModule()));
    registerBuiltin(std::unique_ptr<Primitive>(new Checkra1nExploitModule()));
}

void PrimitiveRegistry::registerBootChainPrimitives() {
    registerBuiltin(std::unique_ptr<Primitive>(new RecoveryUploadPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new RecoveryBootChainPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new PongoProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new PongoBootChainPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new RamdiskShellPrimitive()));
}

void PrimitiveRegistry::registerHostProbePrimitives() {
    registerBuiltin(std::unique_ptr<Primitive>(new OfflinePatchPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new IpswdHostProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new SandboxCapabilityProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new AfcInjectionPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new NormalModeProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new BackupProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new TssSigningProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new MobileBackup2ProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new CodesignSigningProbePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new SideloadPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new DeviceTreeMmioPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new RootlessBootstrapPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new DpkgStorePrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new MedicinePrimitive()));
}

void PrimitiveRegistry::registerBuiltins() {
    if (mBuiltinsRegistered) {
        return;
    }

    registerBootromPrimitives();
    registerGen6ChainPrimitives();
    registerHistoricalExploitModules();
    registerBootChainPrimitives();
    registerHostProbePrimitives();

    mBuiltinsRegistered = true;
}

std::vector<Primitive*> PrimitiveRegistry::list() const {
    std::vector<Primitive*> out;
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        out.push_back(mPrimitives[i].get());
    }
    return out;
}

std::vector<Primitive*> PrimitiveRegistry::list(PrimitiveCategory category) const {
    std::vector<Primitive*> out;
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        if (mPrimitives[i]->category() == category) {
            out.push_back(mPrimitives[i].get());
        }
    }
    return out;
}

Primitive* PrimitiveRegistry::findByName(const std::string& name) const {
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        if (name == mPrimitives[i]->name()) {
            return mPrimitives[i].get();
        }
    }
    return nullptr;
}

} /* namespace primitives */
} /* namespace PP */
