#pragma once

#include "../common/Common.h"

namespace GameboyEmu::Debugger {
	struct StacktraceEntry {
		word callee;
		word dest_address;
		word ret_address;
		byte page;
	};
}