#include "SimProgram.hpp"

using namespace std;

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

// ########## GLOBAL WAREHOUSE FOR MATERIAL ##########
material_warehouse warehouse("Material warehouse", (float)MATERIAL_WAREHOUSE_CAPACITY, (float)INITIAL_MATERIAL_WAREHOUSE_WEIGHT);

// ########## MACHINES ##########
machine pressing_machine(maintenance_time[0][0], "Pressing machine");
machine one_sided_sander(maintenance_time[1][0], "One sided sander");
machine aligner(maintenance_time[2][0], "Aligner");
machine stretcher(maintenance_time[3][0], "Stretcher");
machine double_sided_sander(maintenance_time[4][0], "Double sided sander");
machine oiling_machine(maintenance_time[5][0], "Oiling machine");
machine *machines[6];

// ########## WORKERS ##########

worker pressing_machine_worker(break_time[0][0], "Pressing machine worker");
worker one_sided_sander_worker(break_time[1][0], "One sided sander worker");
worker aligner_worker(break_time[2][0], "Aligner worker");
worker stretcher_worker(break_time[3][0], "Stretcher worker");
worker double_sided_sander_worker(break_time[4][0], "Double sided sander worker");
worker oiling_machine_worker(break_time[5][0], "Oiling machine worker");
worker *workers[6];
// TODO: add 10 packing workers.

// ########## Queue for orders ##########
Queue palettes_queue("Brake discs pallete:"); 

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- FACILITIES --------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########## Material warehouse ##########
material_warehouse::material_warehouse(string desc, float max_capacity, float current_capacity) : Facility()
{
    this->max = max_capacity;
    this->current = current_capacity;
}

float material_warehouse::get_current() { return this->current; }

bool material_warehouse::add_material(float amount)
{
    if (this->current + amount > this->max)
    {
        // cout << "WAREHOUSE: warehouse full - " << this->current + amount - this->max << " kg not added" << endl; // TODO: ...
        this->current = this->max;
        return false;
    }

    this->current += amount; // increase the capacity of the warehouse by the amount of material

    return true;
}

bool material_warehouse::use_material(float amount)
{
    if (amount > this->current) // chceck if the amount is not bigger than the capacity
    {
        // cout << "WAREHOUSE: amount of material is bigger than amount in wh" << endl; // TODO: ...
        return false;
    }
    this->current -= amount; // decrease the capacity of the warehouse by the amount of material

    float utilization = static_cast<float>(this->current) / static_cast<float>(this->max) * 100;

    // TODO:

    return true;
}

// ########## Machines for producing brake discs ##########
machine::machine(float time, string name) : input_queue(), Facility()
{
    this->maintenance_time = time;
    this->name = name;
}

float machine::get_maintenance_time()
{
    return this->maintenance_time;
}

string machine::get_name()
{
    return this->name;
}

// ########## Workers ##########
worker::worker(float break_time, string name) : Facility()
{
    this->break_time = break_time;
    this->name_of_worker = name;
}

float worker::get_break_time()
{
    return this->break_time;
}

string worker::get_name_of_worker()
{
    return this->name_of_worker;
}

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- PROCESSES ---------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########## Simulation proccess for the production of the palettes of brake discs ##########
palette::palette(unsigned amount) : Process()
{
    this->palette_size = amount;
    this->palette_id = palette::palette_count++;
    this->startTime = Time;
}

void palette::Behavior()
{
    palettes_queue.Insert(this); // insert the order into the queue
    cout << "Palette:" << endl;
    cout << "\tPalette id: " << this->palette_id << endl;
    cout << "\tPalette size: " << this->palette_size << endl;
    cout << "\tStart time: " << this->startTime / SECONDS_IN_HOUR << endl;
    cout << "\tEnd time: " << Time / SECONDS_IN_HOUR << endl;
}

// ########## Simulation proccess for the maintenance of the machine ##########
maintenance::maintenance(machine *machine) : Process()
{
    this->machine_to_maintain = machine;
}

void maintenance::Behavior()
{
    // cout << "\tSTART " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_MINUTE << endl;

    Seize(*(this->machine_to_maintain));
    Wait(Normal((this->machine_to_maintain)->get_maintenance_time(), (this->machine_to_maintain)->get_maintenance_time() * 0.1)); // 10% dispersion
    Release(*(this->machine_to_maintain));

    // cout << "\tEND " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Simulation proccess for the break of the worker ##########
break_worker::break_worker(worker *worker) : Process()
{
    this->worker_to_break = worker;
}

void break_worker::Behavior()
{
    //cout << "\tSTART OF BREAK " << (this->worker_to_break)->get_name_of_worker() << " time " << Time / SECONDS_IN_MINUTE << endl;

    Seize(*(this->worker_to_break));
    Wait(Normal((this->worker_to_break)->get_break_time(), (this->worker_to_break)->get_break_time() * 0.1)); // 10% dispersion
    Release(*(this->worker_to_break));

    //cout << "\tEND OF BREAK " << (this->worker_to_break)->get_name_of_worker() << " time " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Simulation proccess for the order ##########
Order::Order() : Process()
{
    this->order_size = static_cast<unsigned>(Uniform(ORDER_SIZE_MIN, ORDER_SIZE_MAX)); // random number of brake discs in the order
    this->amount_of_material = PIECE_MATERIAL_WEIGHT * this->order_size;               // amount of material needed for the order
    this->order_id = Order::order_count++;                                             // id of the order
}

void Order::Behavior()
{
    cout << "Order:" << endl;
    cout << "\tOrder came in: " << Time / SECONDS_IN_HOUR << endl;
    cout << "\tOrder size: " << this->order_size << endl;
    cout << "\tAmount of material: " << this->amount_of_material << endl;

    Wait(SECONDS_IN_MINUTE);

    this->startTime = Time; // save the time of order

    if (warehouse.use_material(this->amount_of_material)) // chcek if there is enough material for the order 
    {    
        for (;;)
        {
            if (this->order_size > 5000)
            {
                (new palette(5000))->Activate(); // create new palette
                this->order_size -= 5000;
            }
            else
            {
                (new palette(this->order_size))->Activate(); // create new palette
                break;
            }
        }
        //machines[0]->input_queue.Insert(this); // insert the order into the queue of the first machine
    }
    else
    {
        // TODO: not enough material mabe the retunr is worng
        // cout << "ORDER N." << this->order_id << ": not enough material" << endl;
        return;
    }

    // create new batch
}

// ########## Process of supply of material ##########
void Supply::Behavior()
{
    // cout << "Supply of 5000 " << endl;

    if (warehouse.add_material(MATERIAL_SUPPLY_WEIGHT))
    {
        // cout << "SUPPLY: added 5000 kg of material" << endl;
    }
    else
    {
        return;
    }
}

//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
void maintenance_event::Behavior()
{
    Activate(Time + Normal(SECONDS_IN_HOUR * 8, 0)); // schedule the next maintenance event after 8 hours

    for (int i = 0; i < 6; i++)
    {
        (new maintenance(machines[i]))->Activate(); // activate the maintenance process for each machine
    }

    // cout << "Maintenance event time: " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Event for the break ##########
void break_event::Behavior()
{
    Activate(Time + Normal(SECONDS_IN_HOUR * 2, SECONDS_IN_MINUTE * 3)); // schedule the next break event after 2 hours and with some +- 5 minute dispersion

    for (int i = 0; i < 6; i++)
    {
        (new break_worker(workers[i]))->Activate(); // activate the break process for each worker
    }

    //cout << "Break event time: " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Generator for new orders ##########
void order_event::Behavior()
{
    (new Order)->Activate();
    Activate(Time + Exponential(SECONDS_IN_HOUR * 8)); // order every 8 hours (exponential)
}

// ########## Generator for supplies ##########
void supply_event::Behavior()
{
    (new Supply)->Activate();
    Activate(Time + Normal(SECONDS_IN_DAY, SECONDS_IN_HOUR * 2)); // supply every 24 hours (exponential
}

// --------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- FUNCS --------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //
void fill_machine_array()
{
    machines[0] = &pressing_machine;
    machines[1] = &one_sided_sander;
    machines[2] = &aligner;
    machines[3] = &stretcher;
    machines[4] = &double_sided_sander;
    machines[5] = &oiling_machine;
}

void fill_worker_array()
{
    workers[0] = &pressing_machine_worker;
    workers[1] = &one_sided_sander_worker;
    workers[2] = &aligner_worker;
    workers[3] = &stretcher_worker;
    workers[4] = &double_sided_sander_worker;
    workers[5] = &oiling_machine_worker;
}

void help(const char *prog_name)
{
    // cout << prog_name << " program implements the SHO of the engineering company." << endl;
    // cout << "Subject: IMS" << endl;
    // cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    // cout << "          Adam Pap (xpapad11) " << endl;
    // cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    // cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

int main(int argc, char *argv[])
{

    // parameter handling
    if (argc > 1)
    {
        help(argv[0]);
    }

    fill_machine_array();
    fill_worker_array();

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, SECONDS_IN_DAY * 2); // time of simulation
    (new order_event)->Activate();
    (new supply_event)->Activate();
    (new maintenance_event)->Activate(SECONDS_IN_HOUR * 8); // first maintenance after 8 hours not at the start of the simulation
    (new break_event)->Activate(SECONDS_IN_HOUR * 2);       // first break after 2 hours not at the start of the simulation
    Run();

    // print out statistics TODO: ...

    // cout << "Warehouse cap: " << warehouse.get_current() << endl;

    return ErrCode(SUCCESS);
}

// ended at Many fixes + machine_maintenance & machine classes added. Removed pre…