#!/bin/bash
export CMAKE_PREFIX_PATH=/c/xmr-stak-dep/hwloc:/c/xmr-stak-dep/libmicrohttpd:/c/xmr-stak-dep/openssl:/c/AMD/ADL_SDK/include


#ARGS="-DCUDA_ENABLE=OFF -DOpenCL_ENABLE=OFF"
ARGS="-DCUDA_ENABLE=OFF"

cmake $ARGS -G "Visual Studio 15 2017 Win64" -T v141,host=x64 ..
