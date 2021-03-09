# Description
This folder contains the algorithms and their header files. The algorithms follow a common form dictated by the "algo_template" file and its header; the function prototypes and formats dictated by this pair of files must be strictly followed.

# Algo Requirements
The algorithm must include:
- An initialization function that accepts a pointer to algo_params_t, a union defined in fad_defs.h. The initialization function should create any variables / allocations necessary for proper functionality. Make sure to include a read_size so the calling portion of the program can dictate how long the algorithm should run for.
- An algorithm function that calculates the output based on the input. The parameters and their descriptions are found in the function prototype in algo_template.h. It is called "algo_template" in said file.
- A deinitialization function that dealoccates any memory or storage created during initialization.
