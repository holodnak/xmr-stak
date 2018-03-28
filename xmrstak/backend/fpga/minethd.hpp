#pragma once

#include "xmrstak/jconf.hpp"
#include "xmrstak/backend/cpu/crypto/cryptonight.h"
#include "xmrstak/backend/fpga/jconf.hpp"
#include "xmrstak/backend/miner_work.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/cryptonight.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <future>

namespace xmrstak
{
	namespace fpga
	{

	class minethd : public iBackend
	{
	public:
		static bool thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id);
		static cryptonight_ctx * minethd_alloc_ctx();
		static void switch_work(miner_work& pWork);
		static std::vector<iBackend*> thread_starter(uint32_t threadOffset, miner_work& pWork);
		static bool init_fpgas();
		static bool self_test();

	public:
		typedef void(*cn_hash_fun)(const void*, size_t, void*, cryptonight_ctx*);
		static cn_hash_fun func_selector(bool bHaveAes, bool bNoPrefetch, xmrstak_algo algo);

	private:
		minethd(miner_work& pWork, size_t iNo, int iMultiway, bool no_prefetch, int64_t affinity);

		void work_main();
		void consume_work();

		uint64_t iJobNo;

		static miner_work oGlobalWork;
		miner_work oWork;
		uint32_t Nonce;

		std::promise<void> order_fix;
		std::mutex thd_aff_set;

		std::thread oWorkThd;
		int64_t affinity;

		int iComPort;
		bool bQuit;
		bool bNoPrefetch;

	};

} // namespace cpu
} // namepsace xmrstak
