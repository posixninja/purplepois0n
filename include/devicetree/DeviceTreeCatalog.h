/*
 * DeviceTreeCatalog.h
 *
 * Extract MMIO catalog from IPSW / DeviceTree via ipsw dtree.
 */

#ifndef DEVICETREE_DEVICE_TREE_CATALOG_H_
#define DEVICETREE_DEVICE_TREE_CATALOG_H_

#include "devicetree/MmioTypes.h"

namespace PP {
namespace devicetree {

/** Run ipsw dtree -j on @p inputPath (IPSW, im4p, or raw DT). */
DeviceTreeCatalog buildCatalogFromPath(const std::string& inputPath, bool includeAllRegions);

/** Parse ipsw dtree JSON already captured on disk. */
DeviceTreeCatalog buildCatalogFromJsonFile(const std::string& jsonPath, bool includeAllRegions);

/** Parse ipsw dtree JSON text in memory. */
DeviceTreeCatalog buildCatalogFromJsonText(const std::string& jsonText, bool includeAllRegions);

void logCatalogSummary(const DeviceTreeCatalog& catalog);
void logAgxMmioHints(const DeviceTreeCatalog& catalog);
void logFullRegisterInventory(const DeviceTreeCatalog& catalog, size_t maxEntries = 0);

bool writeCatalogJson(const DeviceTreeCatalog& catalog, const std::string& outPath, std::string* error);

/** Last catalog built in-process (CLI / primitive). */
const DeviceTreeCatalog* globalCatalog();
void setGlobalCatalog(const DeviceTreeCatalog& catalog);

/** PURPLEPOIS0N_MMIO_CATALOG or explicit path. */
bool loadGlobalCatalogFromEnv();

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_DEVICE_TREE_CATALOG_H_ */
