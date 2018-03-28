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
		enum {
			CONN_NONE = 0,
			CONN_UART = 1
		};

		typedef struct {

			//device handle (from fpga::open)
			void *handle;

			//inputs
			int device_id;
			const char *device_name;
			int device_version;
			int device_com_port;

			//outputs
			uint32_t nonce;

			////////////////////////////////////////////
			int device_arch[2];
			int device_mpcount;
			int device_blocks;
			int device_threads;
			int device_bfactor;
			int device_bsleep;
			int syncMode;

			uint32_t *d_input;
			uint32_t inputlen;
			uint32_t *d_result_count;
			uint32_t *d_result_nonce;
			uint32_t *d_long_state;
			uint32_t *d_ctx_state;
			uint32_t *d_ctx_state2;
			uint32_t *d_ctx_a;
			uint32_t *d_ctx_b;
			uint32_t *d_ctx_key1;
			uint32_t *d_ctx_key2;
			uint32_t *d_ctx_text;
			std::string name;
			size_t free_device_memory;
			size_t total_device_memory;
		} fpga_ctx;

		bool fpga_open(fpga_ctx *ctx);
		bool fpga_close(fpga_ctx *ctx);

		int fpga_get_devicecount(int *c);
		int fpga_get_deviceinfo(fpga_ctx *ctx);

	} // namespace fpga
} // namepsace xmrstak
