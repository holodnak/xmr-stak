
#include "fpga.hpp"
#include "serial.hpp"

namespace xmrstak
{
	namespace fpga
	{
		int num_devices;

		bool fpga_init()
		{
			int i;

			//find devices.  we will cheat
			num_devices = 2;
			return true;
		}

		void fpga_kill()
		{

		}

		bool fpga_open(fpga_ctx *ctx)
		{
			char str[32];

			sprintf(str, "COM%1d", ctx->device_com_port);

			if (serial::open(str) == false) {
				printer::inst()->print_msg(L0, "FPGA: failed to open FPGA at %s.", str);
				return(false);
			}

			printer::inst()->print_msg(L0, "FPGA: opened FPGA at %s.", str);
			return true;
		}

		bool fpga_close(fpga_ctx *ctx)
		{
			serial::close(ctx->handle);
			return true;
		}


		int fpga_get_devicecount(int *c)
		{
			*c = num_devices;

			//no error
			return 1;
		}

		int fpga_get_deviceinfo(fpga_ctx *ctx)
		{
			if (ctx->device_id == 0) {
				ctx->device_name = "deadhasher";
				ctx->device_version = 10;
				ctx->device_com_port = 3;
			}
			if (ctx->device_id == 1) {
				ctx->device_name = "deadhasher_xilinx";
				ctx->device_version = 10;
				ctx->device_com_port = 5;
			}

			//no error
			return 1;
		}

	} // namespace fpga
} // namepsace xmrstak
