#pragma once

#include <windows.h>
#include <vector>

namespace psvcd {

bool PrintSDStatus(const std::vector<BYTE>& bytes);

bool PrintSD_CMD3Status(const std::vector<BYTE>& bytes);

};
