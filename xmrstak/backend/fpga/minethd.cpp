/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */


#include "../cpu/crypto/cryptonight_aesni.h"

#include "xmrstak/misc/console.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend//globalStates.hpp"
#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/params.hpp"
#include "jconf.hpp"
#include "fpga.hpp"

#include "xmrstak/misc/executor.hpp"
#include "minethd.hpp"
#include "xmrstak/jconf.hpp"

#include "../cpu/hwlocMemory.hpp"
#include "xmrstak/backend/miner_work.hpp"

#include "autoAdjust.hpp"

#include <assert.h>
#include <cmath>
#include <chrono>
#include <cstring>
#include <thread>
#include <bitset>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>

#if defined(__APPLE__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif //__APPLE__

#endif //_WIN32


namespace xmrstak
{
namespace fpga
{
	#ifdef WIN32
	HINSTANCE lib_handle;
#else
	void *lib_handle;
#endif

	bool minethd::thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id)
	{
#if defined(_WIN32)
		// we can only pin up to 64 threads
		if (cpu_id < 64)
		{
			return SetThreadAffinityMask(h, 1ULL << cpu_id) != 0;
		}
		else
		{
			printer::inst()->print_msg(L0, "WARNING: Windows supports only affinity up to 63.");
			return false;
		}
#elif defined(__APPLE__)
		thread_port_t mach_thread;
		thread_affinity_policy_data_t policy = { static_cast<integer_t>(cpu_id) };
		mach_thread = pthread_mach_thread_np(h);
		return thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1) == KERN_SUCCESS;
#elif defined(__FreeBSD__)
		cpuset_t mn;
		CPU_ZERO(&mn);
		CPU_SET(cpu_id, &mn);
		return pthread_setaffinity_np(h, sizeof(cpuset_t), &mn) == 0;
#elif defined(__OpenBSD__)
		printer::inst()->print_msg(L0, "WARNING: thread pinning is not supported under OPENBSD.");
#else
		cpu_set_t mn;
		CPU_ZERO(&mn);
		CPU_SET(cpu_id, &mn);
		return pthread_setaffinity_np(h, sizeof(cpu_set_t), &mn) == 0;
#endif
	}

	cryptonight_ctx* minethd::minethd_alloc_ctx()
	{
		cryptonight_ctx* ctx;
		alloc_msg msg = { 0 };

		switch (::jconf::inst()->GetSlowMemSetting())
		{
		case ::jconf::never_use:
			ctx = cryptonight_alloc_ctx(1, 1, &msg);
			if (ctx == NULL)
				printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
			return ctx;

		case ::jconf::no_mlck:
			ctx = cryptonight_alloc_ctx(1, 0, &msg);
			if (ctx == NULL)
				printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
			return ctx;

		case ::jconf::print_warning:
			ctx = cryptonight_alloc_ctx(1, 1, &msg);
			if (msg.warning != NULL)
				printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
			if (ctx == NULL)
				ctx = cryptonight_alloc_ctx(0, 0, NULL);
			return ctx;

		case ::jconf::always_use:
			return cryptonight_alloc_ctx(0, 0, NULL);

		case ::jconf::unknown_value:
			return NULL; //Shut up compiler
		}

		return nullptr; //Should never happen
	}


	minethd::minethd(miner_work& pWork, size_t iNo, int com_port, bool no_prefetch, int64_t affinity)
	{
		this->backendType = iBackend::FPGA;
		oWork = pWork;
		bQuit = 0;
		iThreadNo = (uint8_t)iNo;
		iJobNo = 0;
		iComPort = com_port;
		bNoPrefetch = no_prefetch;
		this->affinity = affinity;

		std::unique_lock<std::mutex> lck(thd_aff_set);
		std::future<void> order_guard = order_fix.get_future();
		oWorkThd = std::thread(&minethd::work_main, this);
		order_guard.wait();

		if (affinity >= 0) //-1 means no affinity
			if (!thd_setaffinity(oWorkThd.native_handle(), affinity))
				printer::inst()->print_msg(L1, "FPGA: WARNING setting affinity failed.");


	}

	bool minethd::init_fpgas()
	{
		printer::inst()->print_msg(L0, "FPGA: initialized ok.");
		return(true);
	}

	bool minethd::self_test()
	{
		cryptonight_ctx* ctx0;
		unsigned char out[32];
		bool bResult = true;

		ctx0 = new cryptonight_ctx;
		if (::jconf::inst()->HaveHardwareAes())
		{
			//cryptonight_hash_ctx("This is a test", 14, out, ctx0);
			bResult = memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 32) == 0;
		}
		else
		{
			//cryptonight_hash_ctx_soft("This is a test", 14, out, ctx0);
			bResult = memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 32) == 0;
		}
		delete ctx0;

//		if(!bResult)
			printer::inst()->print_msg(L0,
			"FPGA: Cryptonight hash self-test failed. This might be caused by bad compiler optimizations.");

		return bResult;
	}


	std::vector<iBackend*> minethd::thread_starter(uint32_t threadOffset, miner_work& pWork)
	{
		std::vector<iBackend*> pvThreads;

		if (!configEditor::file_exist(params::inst().configFileFPGA))
		{
			autoAdjust adjust;
			if (!adjust.printConfig())
				return pvThreads;
		}

		if (!jconf::inst()->parse_config())
		{
			win_exit();
		}

		if (!init_fpgas())
		{
			printer::inst()->print_msg(L1, "WARNING: FPGA device not found");
			return pvThreads;
		}

		//Launch the requested number of single and double threads, to distribute
		//load evenly we need to alternate single and double threads
		size_t i, n = jconf::inst()->GetThreadCount();
		pvThreads.reserve(n);

		jconf::thd_cfg cfg;
		for (i = 0; i < n; i++)
		{
			jconf::inst()->GetThreadConfig(i, cfg);

			if (cfg.iCpuAff >= 0)
			{
#if defined(__APPLE__)
				printer::inst()->print_msg(L1, "WARNING on macOS thread affinity is only advisory.");
#endif

				printer::inst()->print_msg(L1, "FPGA: Starting thread for device %d, affinity: %d.", i, (int)cfg.iCpuAff);
			}
			else
				printer::inst()->print_msg(L1, "FPGA: Starting thread for device %d (COM %d), no affinity.", i, cfg.iComPort);

			minethd* thd = new minethd(pWork, i + threadOffset, cfg.iComPort, cfg.bNoPrefetch, cfg.iCpuAff);
			pvThreads.push_back(thd);
			}

		return pvThreads;
		}

	void minethd::switch_work(miner_work& pWork)
	{
		// iConsumeCnt is a basic lock-like polling mechanism just in case we happen to push work
		// faster than threads can consume them. This should never happen in real life.
		// Pool cant physically send jobs faster than every 250ms or so due to net latency.

		while (globalStates::inst().iConsumeCnt.load(std::memory_order_seq_cst) < globalStates::inst().iThreadCount)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		globalStates::inst().oGlobalWork = pWork;
		globalStates::inst().iConsumeCnt.store(0, std::memory_order_seq_cst);
		globalStates::inst().iGlobalJobNo++;
	}

	void minethd::consume_work()
	{
		memcpy(&oWork, &globalStates::inst().oGlobalWork, sizeof(miner_work));
		iJobNo++;
		globalStates::inst().iConsumeCnt++;
	}

	minethd::cn_hash_fun minethd::func_selector(bool bHaveAes, bool bNoPrefetch, xmrstak_algo algo)
	{
		// We have two independent flag bits in the functions
		// therefore we will build a binary digit and select the
		// function as a two digit binary

		uint8_t algv;
		switch (algo)
		{
		case cryptonight:
			algv = 2;
			break;
		case cryptonight_lite:
			algv = 1;
			break;
		case cryptonight_monero:
			algv = 0;
			break;
		case cryptonight_heavy:
			algv = 3;
			break;
		default:
			algv = 2;
			break;
		}

		static const cn_hash_fun func_table[] = {
			cryptonight_hash<cryptonight_monero, false, false>,
			cryptonight_hash<cryptonight_monero, true, false>,
			cryptonight_hash<cryptonight_monero, false, true>,
			cryptonight_hash<cryptonight_monero, true, true>,
			cryptonight_hash<cryptonight_lite, false, false>,
			cryptonight_hash<cryptonight_lite, true, false>,
			cryptonight_hash<cryptonight_lite, false, true>,
			cryptonight_hash<cryptonight_lite, true, true>,
			cryptonight_hash<cryptonight, false, false>,
			cryptonight_hash<cryptonight, true, false>,
			cryptonight_hash<cryptonight, false, true>,
			cryptonight_hash<cryptonight, true, true>,
			cryptonight_hash<cryptonight_heavy, false, false>,
			cryptonight_hash<cryptonight_heavy, true, false>,
			cryptonight_hash<cryptonight_heavy, false, true>,
			cryptonight_hash<cryptonight_heavy, true, true>
		};

		std::bitset<2> digit;
		digit.set(0, !bHaveAes);
		digit.set(1, !bNoPrefetch);

		return func_table[algv << 2 | digit.to_ulong()];
	}

	void minethd::work_main()
	{
		if (affinity >= 0) //-1 means no affinity
			bindMemoryToNUMANode(affinity);

		order_fix.set_value();
		std::unique_lock<std::mutex> lck(thd_aff_set);
		lck.release();
		std::this_thread::yield();

		cn_hash_fun hash_fun;
		cryptonight_ctx* ctx;
		uint64_t iCount = 0;
		uint64_t* piHashVal;
		uint32_t* piNonce;
		job_result result;

		printer::inst()->print_msg(L1, "FPGA: initializing COM %d", iComPort);

		hash_fun = func_selector(::jconf::inst()->HaveHardwareAes(), bNoPrefetch, ::jconf::inst()->GetMiningAlgo());
		ctx = minethd_alloc_ctx();

		piHashVal = (uint64_t*)(result.bResult + 24);
		piNonce = (uint32_t*)(oWork.bWorkBlob + 39);
		globalStates::inst().inst().iConsumeCnt++;
		result.iThreadId = iThreadNo;

		while (bQuit == 0)
		{
			if (oWork.bStall)
			{
				/* We are stalled here because the executor didn't find a job for us yet,
				* either because of network latency, or a socket problem. Since we are
				* raison d'etre of this software it us sensible to just wait until we have something
				*/

				while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

				consume_work();
				continue;
			}

			size_t nonce_ctr = 0;
			constexpr size_t nonce_chunk = 4096; // Needs to be a power of 2

			assert(sizeof(job_result::sJobID) == sizeof(pool_job::sJobID));
			memcpy(result.sJobID, oWork.sJobID, sizeof(job_result::sJobID));

			if (oWork.bNiceHash)
				result.iNonce = *piNonce;

			if (::jconf::inst()->GetMiningAlgo() == cryptonight_monero)
			{
				if (oWork.bWorkBlob[0] >= 7)
					hash_fun = func_selector(::jconf::inst()->HaveHardwareAes(), bNoPrefetch, cryptonight_monero);
				else
					hash_fun = func_selector(::jconf::inst()->HaveHardwareAes(), bNoPrefetch, cryptonight);
			}

			if (::jconf::inst()->GetMiningAlgo() == cryptonight_heavy)
			{
				if (oWork.bWorkBlob[0] >= 3)
					hash_fun = func_selector(::jconf::inst()->HaveHardwareAes(), bNoPrefetch, cryptonight_heavy);
				else
					hash_fun = func_selector(::jconf::inst()->HaveHardwareAes(), bNoPrefetch, cryptonight);
			}

			uint32_t rawIntensity = 512;
			uint32_t h_per_round = rawIntensity;
			size_t round_ctr = 0;

			//send data to the fpga

			//repeat until the job number changes
			while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
			{

				//Allocate a new nonce every 64 rounds
				if ((round_ctr++ & 0x3F) == 0)
				{
					globalStates::inst().calc_start_nonce(Nonce, oWork.bNiceHash, h_per_round * 16);
				}

				unsigned int results[0x100];
				memset(results, 0, sizeof(unsigned int)*(0x100));

//				XMRRunJob(pGpuCtx, results, miner_algo, version);

				for (size_t i = 0; i < results[0xFF]; i++)
				{
					uint8_t	bWorkBlob[112];
					uint8_t	bResult[32];

					memcpy(bWorkBlob, oWork.bWorkBlob, oWork.iWorkSize);
					memset(bResult, 0, sizeof(job_result::bResult));

					*(uint32_t*)(bWorkBlob + 39) = results[i];

					hash_fun(bWorkBlob, oWork.iWorkSize, bResult, ctx);
					if ((*((uint64_t*)(bResult + 24))) < oWork.iTarget)
						executor::inst()->push_event(ex_event(job_result(oWork.sJobID, results[i], bResult, iThreadNo), oWork.iPoolId));
					else
						executor::inst()->push_event(ex_event("FPGA Invalid Result", iComPort, oWork.iPoolId));
				}

				iCount += rawIntensity;
				uint64_t iStamp = get_timestamp_ms();
				iHashCount.store(iCount, std::memory_order_relaxed);
				iTimestamp.store(iStamp, std::memory_order_relaxed);
				std::this_thread::yield();
			}
			
#if 0
			while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
			{
				if ((iCount++ & 0xF) == 0) //Store stats every 16 hashes
				{
					uint64_t iStamp = get_timestamp_ms();
					iHashCount.store(iCount, std::memory_order_relaxed);
					iTimestamp.store(iStamp, std::memory_order_relaxed);
				}

				if ((nonce_ctr++ & (nonce_chunk - 1)) == 0)
				{
					globalStates::inst().calc_start_nonce(result.iNonce, oWork.bNiceHash, nonce_chunk);
				}

				*piNonce = result.iNonce;

			//	hash_fun(oWork.bWorkBlob, oWork.iWorkSize, result.bResult, ctx);

				if (*piHashVal < oWork.iTarget)
					executor::inst()->push_event(ex_event(result, oWork.iPoolId));
				result.iNonce++;

				std::this_thread::yield();
			}
#endif
			consume_work();
		}

		cryptonight_free_ctx(ctx);
	}


} // namespace fpga
} // namepsace xmrstak
