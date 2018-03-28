R"===(
/*
 * FPGA configuration. You should play around with intensity and worksize as the fastest settings will vary.
 * index         - FPGA index number usually starts from 0
 * affine_to_cpu - This will affine the thread to a CPU. This can make a FPGA miner play along nicer with a CPU miner.
 * "fpga_threads_conf" :
 * [
 *     { "com_port" : 3, "no_prefetch" : true, "affine_to_cpu" : false },
 * ],
 * If you do not wish to mine with your FPGA(s) then use:
 * "fpga_threads_conf" :
 * null,
 */

"fpga_threads_conf" : 
[
FPGACONFIG
],

)==="
