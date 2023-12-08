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
#include <vector>

// time conversion
#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60

// time constants
#define PIECE_REWORK_TIME 18 // in seconds
#define PIECE_CONTROL_TIME 10
#define WORKER_BREAK 15 * SECONDS_IN_MINUTE

// weight constants
#define PIECE_MATERIAL_WEIGHT 0.5              // in kg
#define PIECE_AFTER_PRODUCTION_WEIGHT 0.25     // in kg
#define MATERIAL_SUPPLY_WEIGHT 20000           // in kg. according to https://www.allabouttrucks-cdl.com/2019/08/how-many-tons-of-cargo-can-transport.html
#define MATERIAL_WAREHOUSE_CAPACITY 50000      // in kg
#define INITIAL_MATERIAL_WAREHOUSE_WEIGHT 5000 // in kg

// number amount constants
#define ORDER_SIZE_MIN 1000  // in pieces
#define ORDER_SIZE_MAX 10000 // in pieces
#define PALETTE_PIECES 2000

// chances constants
#define BAD_PIECE_PERCENT 16

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- ENUMS ------------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// error codes enum
enum ErrCode {
    SUCCESS = 0,
    FAIL = 1
};

enum machine_identifier {
    PRESSING_MACHINE = 0,
    ONE_SIDED_SANDER,
    ALIGNER,
    STRETCHER,
    DOUBLE_SIDED_SANDER,
    OILING_MACHINE,
    PACKING,
    QUALITY_CONTROL
};

// ------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------- STRUCTS ------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------- //

struct ProgramOptions {
    bool help = false;
    double SIMULATION_DURATION = 365 * SECONDS_IN_DAY;
    unsigned EXPERIMENT = 0;
    unsigned N_PRESSING = 1;
    bool press = false;
    unsigned N_ONE_SIDED_SANDER = 1;
    bool one_sided = false;
    unsigned N_ALIGNER = 1;
    bool align = false;
    unsigned N_STRETCHER = 1;
    bool stretch = false;
    unsigned N_DOUBLE_SIDED_SANDER = 1;
    bool double_sided = false;
    unsigned N_OILING = 1;
    bool oil = false;
    unsigned N_PACKING_WORKERS = 10;
    bool pack = false;
    unsigned N_QUALITY_CONTROLLERS = 10;
    bool quality = false;
};

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

float maintenance_time[][1] = {
    // first value is the time for
    {40 * SECONDS_IN_MINUTE}, // pressing machine
    {30 * SECONDS_IN_MINUTE}, // one sided sander
    {35 * SECONDS_IN_MINUTE}, // aligner
    {45 * SECONDS_IN_MINUTE}, // stretcher
    {40 * SECONDS_IN_MINUTE}, // double sided sander
    {20 * SECONDS_IN_MINUTE}  // oiling machine
};

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ---------------------------------------------- STORES -------------------------------------------------- //

//----------------------------------------------- FACILITIES ----------------------------------------------- //
class machine_work;

// ########################################### Workers ###########################################
class worker : public Store {
private:
    unsigned n_workers;
    float break_time;
    std::string name_of_worker;
    bool is_break = false;
    std::vector<machine_work *> tasks;

public:
    // constructor
    worker(unsigned n, float bt, std::string name) : Store(n),
                                                     n_workers(n),
                                                     break_time(bt),
                                                     name_of_worker(name) {}

    unsigned get_n_workers() { return n_workers; }

    float get_break_time() { return break_time; }

    void start_break() { is_break = true; };

    void end_break() { is_break = false; };

    bool is_break_time() { return is_break; };

    void start_task(machine_work *task) { tasks.push_back(task); };

    std::vector<machine_work *> get_current_tasks() { return tasks; }

    void remove_task(machine_work *task) {
        for (size_t i = 0; i < tasks.size(); i++) {
            if (tasks[i] == task) {
                tasks.erase(tasks.begin() + i);
                break;
            }
        }
    };

    std::string get_name_of_worker() { return name_of_worker; };
};

// ########################################### Machines for producing brake discs ###########################################
class machine : public Store {
private:
    unsigned store_capacity;
    float maintenance_time;
    float preparation_time;
    float piece_production_time;
    std::string name;
    worker *machine_worker;
    machine_identifier machine_id;

public:
    Queue pallets_waiting;
    unsigned pallets_done = 0;
    double pallets_time_all = 0;

    // constructor
    machine(unsigned sc, float mt, float pt, float ppt, std::string n, worker *w, machine_identifier mi) : Store(sc),
                                                                                                           store_capacity(sc),
                                                                                                           maintenance_time(mt),
                                                                                                           preparation_time(pt),
                                                                                                           piece_production_time(ppt),
                                                                                                           name(n),
                                                                                                           machine_worker(w),
                                                                                                           machine_id(mi),
                                                                                                           pallets_waiting() {}

    unsigned get_store_capacity() { return store_capacity; }

    float get_maintenance_time() { return maintenance_time; }

    float get_preparation_time() { return preparation_time; }

    float get_piece_production_time() { return piece_production_time; }

    std::string get_name() { return name; }

    machine_identifier get_machine_id() { return machine_id; }

    worker *get_worker() { return machine_worker; }

    double get_average_pallet_time() { return pallets_done ? pallets_time_all / pallets_done : 0; }
};

// ########################################### Material warehouse ###########################################
class material_warehouse : public Facility {
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

// ########################################### Simulation proccess for the production of the palettes of brake discs ###########################################
class Order;

class palette : public Process {
private:
    unsigned palette_size; // number of brake discs in the palette
    unsigned palette_done; // brake discs done from the palette
    unsigned palette_id;   // id of the palette
    double startTime;      // time of the start of the palette
    Order *order;          // which order is the palette from
    unsigned bad_pieces = 0;

public:
    static unsigned palette_count; // id of the palette
    // constructor
    palette(unsigned amount_of_brake_discs, Order *po) : Process(),
                                                         palette_size(amount_of_brake_discs),
                                                         palette_done(0),
                                                         palette_id(palette::palette_count++),
                                                         startTime(Time),
                                                         order(po) {}

    unsigned get_palette_size() { return palette_size; }

    unsigned get_palette_done() { return palette_done; }

    unsigned get_palette_id() { return palette_id; }

    void increment_palette_done(unsigned i = 0) { palette_done = i ? palette_done + i : palette_done + 1; }

    unsigned get_bad_pieces() { return bad_pieces; }

    void increment_bad_pieces(unsigned i = 0) { bad_pieces = i ? bad_pieces + i : bad_pieces + 1; }

    void Behavior();
};

// init the palette id
unsigned palette::palette_count = 0;

// ########################################### Simulation proccess for the maintenance of the machine ###########################################
class maintenance : public Process {
private:
    machine *machine_to_maintain;

public:
    // constructor
    maintenance(machine *);

    void Behavior();
};

// ########################################### Simulation proccess for the break of the worker ###########################################
class break_worker : public Process {
private:
    worker *worker_to_break;

public:
    // constructor
    break_worker(worker *w) : Process(1), worker_to_break(w) {}

    void Behavior();
};

// ########################################### Simulation proccess for the order ###########################################
class Order : public Process {
private:
    unsigned order_size; // number of brake discs in the order
    unsigned pieces_done = 0;
    float amount_of_material; // amount of material needed for the order
    unsigned order_id;        // id of the order
    double startTime;

public:
    static unsigned order_count; // id of the order
    // constructor
    Order();

    unsigned get_order_id() { return order_id; }
    unsigned get_order_size() { return order_size; }
    unsigned get_pieces_done() { return pieces_done; }
    void add_pieces_done(unsigned i = 0) { pieces_done += i; }
    void Behavior();
};
unsigned Order::order_count = 0;

// ########################################### Process of supply of material ###########################################
class Supply : public Process {
public:
    Supply() : Process() {}

    void Behavior();
};

// ########################################### Process of machine work ###########################################
class machine_work : public Process {
private:
    machine *machine_to_work;
    palette *palette_in_machine;

public:
    machine_work(machine *machine, palette *palette_in) : Process(), machine_to_work(machine), palette_in_machine(palette_in) {}

    void Behavior();
};

// ########################################### Package ###########################################
class package_for_worker : public Process {
private:
    palette *palette_to_pack;
    int package_id;
    unsigned size;

public:
    // constructor
    package_for_worker(palette *palette, int id, unsigned s) : Process(), palette_to_pack(palette), package_id(id), size(s) {}

    void Behavior();
};

// ########################################### Quality control ###########################################
class quality_control : public Process {
private:
    palette *palette_to_check;

public:
    quality_control(palette *palette) : Process(), palette_to_check(palette) {}
    void Behavior();
};

//----------------------------------------------- EVENTS -----------------------------------------------

// ########################################### Event for the maintenance ###########################################
class maintenance_event : public Event {
    void Behavior();
};

// ########################################### Event for the break ###########################################
class break_event : public Event {
    void Behavior();
};

// ########################################### Event for new orders ###########################################
class order_event : public Event {
    void Behavior();
};

// ########################################### Event for supplies ###########################################
class supply_event : public Event {
    void Behavior();
};

// --------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- FUNCS --------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //
void help(const char *);

void print_stats(ProgramOptions &);
bool parse_args(int, char **, ProgramOptions &);

#endif