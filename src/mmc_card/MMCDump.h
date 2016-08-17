#include <string>
#include <stdint.h>

#include <ftd2xx.h>

namespace psvcd {

bool EnterDumpableMode(FT_HANDLE ftHandle);

bool RawBlockDumpMMCCard(FT_HANDLE ftHandle, uint32_t initialCluster, uint32_t& failedCluster, std::string filePath, bool& lowFreqRequired);

};