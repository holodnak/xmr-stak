#!/bin/bash

ARGS="-DCUDA_ENABLE=OFF"

cmake $ARGS -G "Visual Studio 15 2017 Win64" -T v140,host=x64 ..
