I have implemented mts correctly according to the P2 spec.

- mts.c can be compiled using "make" command and takes one parameter (input.txt) 
- mts will save the simulation output to output.txt following the format defined in section 3.3
    - if mts is invoked multiple times on different input files it will overwrite the existing output.txt file 
- mts has no deadlock or livelock so the simulation can complete within 1 minute 
- mts follows all rules of priority and scheduling to ensure trains cross in the correct order 
- mts uses threads to complete this train scheduling and ensures synchronization using mutexes and conditional variables 
- from the provided input.txt file the estimated run time is 1.006904 seconds to execute