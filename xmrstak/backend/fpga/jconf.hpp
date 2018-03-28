#pragma once

#include "xmrstak/params.hpp"

#include <stdlib.h>
#include <string>

namespace xmrstak
{
namespace fpga
{

	class jconf
	{
	public:
		static jconf* inst()
		{
			if (oInst == nullptr) oInst = new jconf;
			return oInst;
		};

		bool parse_config(const char* sFilename = params::inst().configFileFPGA.c_str());

		struct thd_cfg {
			int iComPort;
			bool bNoPrefetch;
			long long iCpuAff;
		};

		size_t GetThreadCount();
		bool GetThreadConfig(size_t id, thd_cfg &cfg);

	private:
		jconf();
		static jconf* oInst;

		struct opaque_private;
		opaque_private* prv;

	};

} // namespace fpga
} // namepsace xmrstak
