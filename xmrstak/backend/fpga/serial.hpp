#pragma once

namespace xmrstak
{
	namespace fpga
	{
		namespace serial
		{

			void *open(char *path);
			void close(void *dev);
			bool send(void *dev, int len, char *data);
			bool receive(void *dev, int len, char *data);

		} // namespace serial
	} // namespace fpga
} // namepsace xmrstak
