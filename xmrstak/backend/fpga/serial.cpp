
#include <Windows.h>

#include "serial.hpp"

namespace xmrstak
{
	namespace fpga
	{
		namespace serial
		{

			void *open(char *path)
			{
				bool ret = true;
				HANDLE m_hSerialComm;

				if (ret)
				{
					m_hSerialComm = CreateFile(path,
						GENERIC_READ | GENERIC_WRITE,
						0,
						0,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						0);

					if (m_hSerialComm == INVALID_HANDLE_VALUE)
					{
						ret = FALSE;
						if (GetLastError() == ERROR_FILE_NOT_FOUND)
						{
							MessageBox(NULL, ("\"COM5\" file not found."), ("NesDbg"), MB_OK);
						}
						else
						{
							MessageBox(NULL, ("Unknown error initializing COM5"), ("NesDbg"), MB_OK);
						}
					}
				}

				DCB serialConfig = { 0 };
				if (ret)
				{
					serialConfig.DCBlength = sizeof(DCB);

					if (!GetCommState(m_hSerialComm, &serialConfig))
					{
						ret = FALSE;
						MessageBox(NULL, ("Error getting comm state for COM5."), ("NesDbg"), MB_OK);
					}
				}

				if (ret)
				{
					serialConfig.BaudRate = CBR_38400;
					serialConfig.ByteSize = 8;
					serialConfig.StopBits = ONESTOPBIT;
					serialConfig.Parity = ODDPARITY;

					if (!SetCommState(m_hSerialComm, &serialConfig))
					{
						ret = FALSE;
						MessageBox(NULL, ("Error setting comm state for COM5."), ("NesDbg"), MB_OK);
					}
				}

				if (ret)
				{
					COMMTIMEOUTS timeouts = { 0 };

					timeouts.ReadIntervalTimeout = 50;
					timeouts.ReadTotalTimeoutMultiplier = 10;
					timeouts.ReadTotalTimeoutConstant = 5000;
					timeouts.WriteTotalTimeoutMultiplier = 10;
					timeouts.WriteTotalTimeoutConstant = 50;

					if (!SetCommTimeouts(m_hSerialComm, &timeouts))
					{
						ret = FALSE;
						MessageBox(NULL, ("Error setting timeout state for COM5."), ("NesDbg"), MB_OK);
					}
				}

				if (ret)
				{
					// Add short sleep here.  The first serial read/write fails sometimes if it occurs to soon
					// after init.
					Sleep(200);

					// Send a debug echo packet to the NES to verify the connection.
					char* pInitString = "FPGAMINER";
					UINT initStringSize = strlen(pInitString) + 1;

					send(m_hSerialComm, initStringSize, pInitString);


					UINT bytesToReceive = 10;

					char* pOutString = new char[bytesToReceive];

					receive(m_hSerialComm, bytesToReceive, pOutString);

					if (strcmp(pInitString, pOutString))
					{
						ret = FALSE;
						MessageBox(NULL, ("FPGA not connected."), ("NesDbg"), MB_OK);
					}

					delete[] pOutString; 

				}

				if (ret == true)
					return m_hSerialComm;
				else
					return NULL;
			}

			void close(void *dev)
			{
				if (dev)
				{
					CloseHandle((HANDLE)dev);
				}
			}

			bool send(void *dev, int len, char *data)
			{
				HANDLE m_hSerialComm = (HANDLE)dev;
				bool  ret = true;
				DWORD bytesWritten = 0;

				ret = WriteFile(m_hSerialComm, data, len, &bytesWritten, NULL) == TRUE;

//				assert(bytesWritten == numBytes);
				if (bytesWritten != len)
				{
					ret = false;
				}

				return ret;
			}

			bool receive(void *dev, int len, char *data)
			{
				HANDLE m_hSerialComm = (HANDLE)dev;
				bool  ret = true;
				DWORD bytesRead = 0;

				ret = ReadFile(m_hSerialComm, data, len, &bytesRead, NULL);

//				assert(bytesRead == numBytes);
				if (bytesRead != len)
				{
					ret = false;
				}

				return ret;
			}

		} // namespace serial
	} // namespace fpga
} // namepsace xmrstak
