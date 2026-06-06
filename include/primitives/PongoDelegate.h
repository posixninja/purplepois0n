/*
 * PongoDelegate.h
 *
 * External checkra1n spawn and PongoOS USB presence checks.
 */

#ifndef PRIMITIVES_PONGO_DELEGATE_H_
#define PRIMITIVES_PONGO_DELEGATE_H_

#include "PrimitiveTypes.h"

#include <string>

namespace PP {
namespace primitives {

class PongoDelegate {
public:
    /** Resolve checkra1n binary: PURPLEPOIS0N_CHECKRA1N, app bundle, PATH. */
    static std::string resolveCheckra1nPath();

    /** True when PongoOS USB device (05ac:4141) is visible. Requires libusb build. */
    static bool isPongoPresent();

    /**
     * Spawn checkra1n -cp (Pongo shell mode). Probe-only unless @p allowMutation.
     * User must place device in DFU before calling.
     */
    static PrimitiveResult spawnCheckra1nShell(bool allowMutation);

    /** Send a probe command and read stdout (requires open Pongo device). */
    static PrimitiveResult probeShell(std::string* stdoutText);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PONGO_DELEGATE_H_ */
