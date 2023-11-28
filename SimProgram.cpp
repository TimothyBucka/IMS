#include "SimProgram.hpp"

using namespace std;

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

// ########## GLOBAL WAREHOUSE FOR MATERIAL ##########
unsigned order_id_count = 0;
// ########## MACHINES ##########
machine pressing_machine(maintenance_time[0][0], "Pressing machine");
machine one_sided_sander(maintenance_time[1][0], "One sided sander");
machine aligner(maintenance_time[2][0], "Aligner");
machine stretcher(maintenance_time[3][0], "Stretcher");
machine double_sided_sander(maintenance_time[4][0], "Double sided sander");
machine oiling_machine(maintenance_time[5][0], "Oiling machine");
machine *machines[6];
// ########## Queue for orders ##########
Queue orders("Brake discs order:");

material_warehouse warehouse("Material warehouse", (float)MATERIAL_WAREHOUSE_CAPACITY, (float)INITIAL_MATERIAL_WAREHOUSE_WEIGHT);

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

//----------------------------------------------- FACILITIES -----------------------------------------------

// ########## Material warehouse ##########
material_warehouse::material_warehouse(string desc, float max_capacity, float current_capacity) : Facility() {
    this->max = max_capacity;
    this->current = current_capacity;
}

float material_warehouse::get_current() { return this->current; }

bool material_warehouse::add_material(float amount) {
    if (this->current + amount > this->max) {
        // cout << "WAREHOUSE: warehouse full - " << this->current + amount - this->max << " kg not added" << endl; // TODO: ...
        this->current = this->max;
        return false;
    }

    this->current += amount; // increase the capacity of the warehouse by the amount of material

    return true;
}

bool material_warehouse::use_material(float amount) {
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
machine::machine(float time, string name) : Facility() {
    this->maintenance_time = time;
    this->name = name;
}

float machine::get_maintenance_time() {
    return this->maintenance_time;
}

string machine::get_name() {
    return this->name;
}

//----------------------------------------------- PROCESSES -----------------------------------------------

// ########## Simulation proccess for the maintenance of the machine ##########
maintenance::maintenance(machine *machine) : Process() {
    this->machine_to_maintain = machine;
}

void maintenance::Behavior() {
    cout << "\tSTART " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_MINUTE << endl;
    Seize(*(this->machine_to_maintain));
    Wait(Normal((this->machine_to_maintain)->get_maintenance_time(), (this->machine_to_maintain)->get_maintenance_time() * 0.1)); // 10% dispersion
    Release(*(this->machine_to_maintain));
    cout << "\tEND " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Simulation proccess for the order ##########
Order::Order() : Process() {
    this->order_size = static_cast<unsigned>(Uniform(ORDER_SIZE_MIN, ORDER_SIZE_MAX)); // random number of brake discs in the order
    this->amount_of_material = PIECE_MATERIAL_WEIGHT * this->order_size;               // amount of material needed for the order
}

void Order::Behavior() {
    // cout << "Order:" << endl;
    // cout << "\tOrder came in: " << Time / SECONDS_IN_HOUR << endl;
    // cout << "\tOrder size: " << this->order_size << endl;
    // cout << "\tAmount of material: " << this->amount_of_material << endl;

    Wait(SECONDS_IN_MINUTE);

    this->startTime = Time; // save the time of order

    this->order_id = order_id_count++;

    if (warehouse.use_material(this->amount_of_material)) {

    } else {
        // TODO: not enough material
        // cout << "ORDER N." << this->order_id << ": not enough material" << endl;
        return;
    }

    // create new batch
}

// ########## Process of supply of material ##########
void Supply::Behavior() {
    // cout << "Supply of 5000 " << endl;

    if (warehouse.add_material(MATERIAL_SUPPLY_WEIGHT)) {

    } else {
        return;
    }
}

//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
void maintenance_event::Behavior() {
    Activate(Time + Normal(SECONDS_IN_HOUR * 8, 0)); // schedule the next maintenance event after 8 hours

    for (int i = 0; i < 6; i++) {
        (new maintenance(machines[i]))->Activate(); // activate the maintenance process for each machine
    }

    cout << "Maintenance event time: " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Generator for new orders ##########
void order_event::Behavior() {
    (new Order)->Activate();
    Activate(Time + Exponential(SECONDS_IN_HOUR * 8)); // order every 8 hours (exponential)
}

// ########## Generator for supplies ##########
void supply_event::Behavior() {
    (new Supply)->Activate();
    Activate(Time + Normal(SECONDS_IN_DAY, SECONDS_IN_HOUR * 2)); // supply every 24 hours (exponential
}

// --------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- FUNCS --------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //
void fill_machine_array() {
    machines[0] = &pressing_machine;
    machines[1] = &one_sided_sander;
    machines[2] = &aligner;
    machines[3] = &stretcher;
    machines[4] = &double_sided_sander;
    machines[5] = &oiling_machine;
}

void help(const char *prog_name) {
    // cout << prog_name << " program implements the SHO of the engineering company." << endl;
    // cout << "Subject: IMS" << endl;
    // cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    // cout << "          Adam Pap (xpapad11) " << endl;
    // cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    // cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

int main(int argc, char *argv[]) {

    // parameter handling
    if (argc > 1) {
        help(argv[0]);
    }

    fill_machine_array();

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, SECONDS_IN_DAY * 2); // time of simulation
    (new order_event)->Activate();
    (new supply_event)->Activate();
    (new maintenance_event)->Activate();
    Run();

    // print out statistics TODO: ...

    // cout << "Warehouse cap: " << warehouse.get_current() << endl;

    return ErrCode(SUCCESS);
}

// ended at Many fixes + machine_maintenance & machine classes added. Removed pre…