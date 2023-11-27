#ifndef SimProgram_hpp
#define SimProgram_hpp

// include libraries
#include <iostream>
#include <simlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>

// error codes enum
enum ErrCode
{
    SUCCESS = 0,
    FAIL = 1
};

enum machine_types
{
    pressing_machine = 0,
    one_sided_sander,
    aligner,
    stretcher,
    double_sided_sander,
    oiling_machine,
    packing,
    // ##### BIG NOT SURE ABOUT IT TODO: ... #####
    transport
};

double maintenance_time[][2] = {
    // first value is the time for the maintenance, second is the dispersion +- 5 minutes or more 
    {40, 5}, // pressing machine
    {30, 5}, // one sided sander
    {35, 5}, // aligner
    {45, 10}, // stretcher
    {40, 5}, // double sided sander
    {20, 5}  // oiling machine
};

#endif