# LOG CHANGE
*  1.00 - cleaned version for final publication on GitHub
*  0.99 - version to be published on OSF
*  0.45 - version with the inheritance of the maximum strength from the parent
*  0.43 - version with TEST_DIVIDER constants in agent rules
*  0.39a - reintroduce spontaneous honor aggressiveness
*  0.38abc - introducing a map of variation measure additionally next to the proportion map and the   average strength map but the variation between a step and its immediate predecessor is very small.
          - Introduced PREVSTEP parameter set to 80 (more often than every 100 steps, it doesn't count in parameter space mode anyway)
          - THE COUNTING OF STEPS IN LONG PARAMETER SPACE EXPERIMENTS has been improved!
          - The ONLY3STRAT parameter was introduced for testing the lean version of the model (no rationals) and deriving PREVSTEP and ONLY3STRAT as calling parameters.
*  0.37a - statistics after action by groups. Other startup defects expanding the statistics of actions listed in the log testing the feasibility of the mortality model
*  0.36b - minor changes to console texts (01/29/2014)
*  0.36a - minor changes in console printouts and related to MSVC++ compilation
         - selection of a simple display in the main window (September 25, 2013)
          
##  TO THE 2013 REPORT
*  0.35c - use of the OptEnumParam class for some calling parameters
*  0.35b - and creating an automatic log and bitmap file name generator.
*  0.35 - the final implementation of the two-level "MAFIA" (relationship to two levels of ancestors counts as a family)
        - Output this as a parameter. Change log name parameters from char * to wb_pchar (you had to extend the OptParam class.
*  0.31 - split into separate sources to implement family honor. "b" - implementation of "mafia"
*  0.30a - various changes to the way text data is output and a third kind of space exploration
*  0.28a- the beginning of creating the third kind of cross-section of the parameter space, and corrections
*  0.27 - space graph selection x polic.effic.
*  0.2631 - Minor fixes ???
*  0.263 - Most parameters handled by the class template from "OptParam"
*  0.262 - IMPORTANT - also WIMPs now look not at their real strength, but at their own reputation, so de facto, apart from childhood, they probably never defend themselves
         - Technical - introducing the use of a parameter class to handle model parameters 
         
##  TO THE 2012 REPORT
*  0.26 - Graph of space - in the proportion of honor and police to each other dividing 1/2 or 2/3 of the total.
        - Another way of coloring "cultures" in the image of the simulation world (red-aggression, green-honor, blue-callPolice)
        -  Trying to change the "crash" schedule for GIVEUP - so they can die, but more destructive to the bullys!?!?
        -  Noise death needed for evolution - NOISE_KILL parameter evolution:
        - local smear random neighbor
                - TODO - random Agent from the whole - DONE FINALLY.
                - TODO? - local reproduction proportional to the strength - REQUIRED ??? 
        - Statistics: how many times the agent is attacked and how many times it won, index clustering for cultures.
        - The frequency of attacks on particular groups over time - no selection, individual profit
        - Large map with police gradient.
        - Possibly various honorary activities. They don't always have to stop, but with probability
        - LATER: the number of police depends on the number of attacks? Another model - tax - NOT THIS TIME. 
*  0.251 - correction of the error in counting "always give up" on the record of the log file
*  0.25 - introducing statistics from the strength of different groups (highest decile only) and print them to a file and to the console.
        - Make the printout of the certificate more flexible so that it is suitable both vertically and horizontally ...
        - INTRODUCING PARAMETER SPACE SEARCH MODE, including RECORDING BITMAP WITH RESULTS
*  0.24 - introduction of the RATIONALITY parameter, because the use of a fully realistic force assessment breaks the selections. Cosmetic changes to the header of the output file
*  0.23 - know their real strength and compare it with the reputation of another when they are to attack or defend themselves. But it didn't work well ...
*  0.20 - first version fully operational
