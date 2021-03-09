# Description
This folder contains the algorithms and their header files. The algorithms follow a common form dictated by the "algo_template" file and its header; the function prototypes and formats dictated by this pair of files must be strictly followed.

# Algo Requirements
To guide the process of creating an algorithm, the creator should start by copying the templates and changing the file names and function names to match the name of the new algorithm.

## Requirements for the algo files:
The algorithm must include:
- An initialization function that accepts a pointer to algo_params_t, a union defined in fad_defs.h. The initialization function should create any variables / allocations necessary for proper functionality. Make sure to include a read_size so the calling portion of the program can dictate how long the algorithm should run for.
- An algorithm function that calculates the output based on the input. The parameters and their descriptions are found in the function prototype in algo_template.h. It is called "algo_template" in said file.
- A deinitialization function that dealoccates any memory or storage created during initialization.

## Requirements for the fad_defs.h:
- The desired structure of the initialization params, inside the algo_params_t union. Follow the format of the other structures in the union.
- Add to the algo_type_t enum with the name of the new algorithm

# List of Algos
The following is a short list and description of the available algorithms. Some are still in the process of being created, or need to be updated to match the template.

## Ready
- algo_template: Outputs a square wave with a set frequency, shifting output volume based on incoming ADC level
## In Progress
- algo_delay: Repeats the microphone input back to the user with a specified time delay
- algo_freq_shift: Takes the microphone input and shifts its incoming frequencies a specified amount. Outputs these shifted frequencies back to the user.
- algo_white: Outputs white noise based on the input level. Louder inputs result in louder white noise.