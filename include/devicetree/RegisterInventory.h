/*
 * RegisterInventory.h
 *
 * Full DeviceTree register / MMIO property inventory.
 */

#ifndef DEVICETREE_REGISTER_INVENTORY_H_
#define DEVICETREE_REGISTER_INVENTORY_H_

#include "devicetree/JsonMini.h"
#include "devicetree/MmioTypes.h"

#include <cstddef>

namespace PP {
namespace devicetree {

struct RegisterInventory {
    DeviceTreeSummary summary;
    std::vector<DtNodeRecord> nodes;
    std::vector<DtRegisterEntry> registers;

    std::vector<MmioRegion> mmioRegions(bool interestingOnly) const;
    size_t countByKind(RegisterKind kind) const;
};

RegisterInventory buildRegisterInventory(const JsonValue& root);

void logRegisterInventorySummary(const RegisterInventory& inventory);
void logRegisterInventoryVerbose(const RegisterInventory& inventory, size_t maxEntries);

void applyInventoryToCatalog(const RegisterInventory& inventory, bool interestingOnly,
                             DeviceTreeCatalog* catalog);

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_REGISTER_INVENTORY_H_ */
