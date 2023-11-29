#ifndef SimProgram_hpp
#define SimProgram_hpp

// include libraries
#include <iostream>
#include <random>
#include <simlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60
#define PIECE_MATERIAL_WEIGHT 0.25  // in kg
#define MATERIAL_SUPPLY_WEIGHT 5000 // in kg

#define MATERIAL_WAREHOUSE_CAPACITY 20000      // in kg
#define INITIAL_MATERIAL_WAREHOUSE_WEIGHT 5000 // in kg

#define ORDER_SIZE_MIN 1000  // in pieces
#define ORDER_SIZE_MAX 10000 // in pieces

// error codes enum
enum ErrCode
{
    SUCCESS = 0,
    FAIL = 1
};

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

float maintenance_time[][1] = {
    // first value is the time for
    {40 * SECONDS_IN_MINUTE},  // pressing machine
    {30 * SECONDS_IN_MINUTE},  // one sided sander
    {35 * SECONDS_IN_MINUTE},  // aligner
    {45 * SECONDS_IN_MINUTE}, // stretcher
    {40 * SECONDS_IN_MINUTE},  // double sided sander
    {20 * SECONDS_IN_MINUTE}   // oiling machine
};

float break_time[][1] = {
    // first value is the time for the break
    {15 * SECONDS_IN_MINUTE}, // pressing machine worker
    {15 * SECONDS_IN_MINUTE}, // one sided sander worker
    {15 * SECONDS_IN_MINUTE}, // aligner worker
    {15 * SECONDS_IN_MINUTE}, // stretcher worker
    {15 * SECONDS_IN_MINUTE}, // double sided sander worker
    {15 * SECONDS_IN_MINUTE}  // oiling machine worker
};

//constants for the process of the making the brake discs


// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

//----------------------------------------------- FACILITIES -----------------------------------------------
// ########## Machines for producing brake discs ##########
class machine : public Facility
{
private:
    float maintenance_time;
    std::string name;

public:
    Queue input_queue; // queue of orders waiting to be processed

    // constructor
    machine(float, std::string);

    float get_maintenance_time();
    std::string get_name();
};

// ########## Workers ##########
class worker : public Facility
{
private:
    float break_time;
    std::string name_of_worker;

public: 
    // constructor
    worker(float, std::string);

    float get_break_time();
    std::string get_name_of_worker();
};

// ########## Material warehouse ##########
class material_warehouse : public Facility
{
private:
    float max;     // max capacity of the warehouse
    float current; // current capacity of the warehouse

public:
    material_warehouse(std::string, float, float);

    float get_max();
    float get_current();

    // send material every 24 hours and add it to the current warehouse capacity
    bool add_material(float);
    bool use_material(float);
};

//----------------------------------------------- PROCESSES -----------------------------------------------

// ########## Simulation proccess for the production of the palettes of brake discs ##########
class palette : public Process
{
private:
    unsigned palette_size; // number of brake discs in the palette
    float startTime; // time of the start of the palette
    unsigned palette_id;   // id of the palette
public:
    static unsigned palette_count; // id of the palette
    // constructor
    palette(unsigned amount_of_brake_discs);

    void Behavior();

};

//init the palette id
unsigned palette::palette_count = 0;

// ########## Simulation proccess for the maintenance of the machine ##########
class maintenance : public Process
{
private:
    machine *machine_to_maintain;

public:
    // constructor
    maintenance(machine *);

    void Behavior();
};

// ########## Simulation proccess for the break of the worker ##########
class break_worker : public Process
{
private: 
    worker *worker_to_break;

public:
    // constructor
    break_worker(worker *);

    void Behavior();
};

// ########## Simulation proccess for the order ##########
class Order : public Process
{
private:
    unsigned order_size;      // number of brake discs in the order
    float amount_of_material; // amount of material needed for the order
    unsigned order_id;        // id of the order
    Queue *orders;
    float startTime;

public:
    static unsigned order_count; // id of the order
    // constructor
    Order();
    void Behavior();
};
unsigned Order::order_count = 0;

// ########## Simulation proccess for the new batch of brake discs from the order ##########
class batch : public Process
{
private:
    unsigned batch_size;           // number of brake discs in the batch to be made
    material_warehouse *warehouse; // pointer to the material warehouse
    Queue *orders;                 // pointer to the queue of orders waiting to be processed

    float startTime; // time of the start of the batch

public:
    void Behavior(); // TODO
};

// ########## Process of supply of material ##########
class Supply : public Process
{
public:
    Supply() : Process() {}

    void Behavior();
};

//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
class maintenance_event : public Event
{
    void Behavior();
};

// ########## Event for the break ##########
class break_event : public Event
{
    void Behavior();
};

// ########## Event for new orders ##########
class order_event : public Event
{
    void Behavior();
};

// ########## Event for supplies ##########
class supply_event : public Event
{
    void Behavior();
};

// --------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- FUNCS --------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //
void fill_machine_array();

void fill_worker_array();

void help(const char *);

#endif