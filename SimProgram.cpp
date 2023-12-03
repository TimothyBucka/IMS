#include "SimProgram.hpp"

using namespace std;

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

// STATISTICS
unsigned number_of_orders = 0;
double time_in_production_sum = 0;

// ########## GLOBAL WAREHOUSE FOR MATERIAL ##########
material_warehouse warehouse("Material warehouse", (float)MATERIAL_WAREHOUSE_CAPACITY, (float)INITIAL_MATERIAL_WAREHOUSE_WEIGHT);

// ########## WORKERS ##########
worker pressing_machine_worker(break_time[0][0], "Pressing machine worker");
worker one_sided_sander_worker(break_time[1][0], "One sided sander worker");
worker aligner_worker(break_time[2][0], "Aligner worker");
worker stretcher_worker(break_time[3][0], "Stretcher worker");
worker double_sided_sander_worker(break_time[4][0], "Double sided sander worker");
worker oiling_machine_worker(break_time[5][0], "Oiling machine worker");
worker *workers[6];

Store packing_workers(N_PACKING_WORKERS);

// ########## MACHINES ##########
machine pressing_machine(maintenance_time[0][0], 60 * SECONDS_IN_MINUTE, 1, "Pressing machine", &pressing_machine_worker, PRESSING_MACHINE);
machine one_sided_sander(maintenance_time[1][0], 10 * SECONDS_IN_MINUTE, 10, "One sided sander", &one_sided_sander_worker, ONE_SIDED_SANDER);
machine aligner(maintenance_time[2][0], 10 * SECONDS_IN_MINUTE, 10, "Aligner", &aligner_worker, ALIGNER);
machine stretcher(maintenance_time[3][0], 50 * SECONDS_IN_MINUTE, 2.5 * SECONDS_IN_MINUTE, "Stretcher", &stretcher_worker, STRETCHER);
machine double_sided_sander(maintenance_time[4][0], 10 * SECONDS_IN_MINUTE, 20, "Double sided sander", &double_sided_sander_worker, DOUBLE_SIDED_SANDER);
machine oiling_machine(maintenance_time[5][0], 0, 2, "Oiling machine", &oiling_machine_worker, OILING_MACHINE);
machine *machines[6];

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- FACILITIES --------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########## Material warehouse ##########
material_warehouse::material_warehouse(string desc, float max_capacity, float current_capacity) : Facility() {
    (void)desc;
    this->max = max_capacity;
    this->current = current_capacity;
}

float material_warehouse::get_current() { return this->current; }

bool material_warehouse::add_material(float amount) {
    if (this->current + amount > this->max) {
        // cout << "########################################### WAREHOUSE #############################################" << endl;
        // cout << "WAREHOUSE: warehouse full - " << this->current + amount - this->max << " kg not added" << endl;
        // cout << "###################################################################################################" << endl;
        this->current = this->max;
        return false;
    }

    // cout << "########################################### WAREHOUSE #############################################" << endl;
    this->current += amount; // increase the capacity of the warehouse by the amount of material
    // cout << "WAREHOUSE: current capacity after material added: " << this->current << endl;
    // cout << "###################################################################################################" << endl;
    return true;
}

bool material_warehouse::use_material(float amount) {
    if (amount > this->current) // chceck if the amount is not bigger than the capacity
    {
        // cout << "########################################### WAREHOUSE #############################################" << endl;
        // cout << "WAREHOUSE: amount of material needed for production is bigger than amount in warehouse" << endl;
        // cout << "###################################################################################################" << endl;

        // TODO: the return might not be enough
        return false;
    }
    this->current -= amount; // decrease the capacity of the warehouse by the amount of material needed for production

    float utilization = static_cast<float>(this->current) / static_cast<float>(this->max) * 100;
    (void)utilization;

    // TODO: print out the utilization of the warehouse in stats

    return true;
}

// ########## Machines for producing brake discs ##########
machine::machine(float time, float prep_time, float piece_time, string name, worker *machine_worker, enum machine_indetifier machine_id) : Facility(), input_queue() {
    this->maintenance_time = time;
    this->preparation_time = prep_time;
    this->piece_production_time = piece_time;
    this->machine_worker = machine_worker;
    this->name = name;
    this->machine_id = machine_id;
}

float machine::get_maintenance_time() {
    return this->maintenance_time;
}

enum machine_indetifier machine::get_machine_id() {
    return this->machine_id;
}

float machine::get_preparation_time() {
    return this->preparation_time;
}

float machine::get_piece_production_time() {
    return this->piece_production_time;
}

string machine::get_name() {
    return this->name;
}

// ########## Workers ##########
worker::worker(float break_time, string name) : Facility() {
    this->break_time = break_time;
    this->name_of_worker = name;
}

float worker::get_break_time() {
    return this->break_time;
}

string worker::get_name_of_worker() {
    return this->name_of_worker;
}

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- PROCESSES ---------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########## Simulation proccess for the production of the palettes of brake discs ##########

void palette::Behavior() {
    // cout << "Palette id: " << this->palette_id << endl;
    // cout << "\tPalette size: " << this->palette_size << endl;
    // cout << "\tStart time: " << this->startTime / SECONDS_IN_MINUTE << endl;

    pressing_machine.input_queue.Insert(this);
    (new machine_work(&pressing_machine, this))->Activate(); // PRESSING MACHINE
    Passivate();

    // transport
    Wait(Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE));

    this->palette_done = 0;
    one_sided_sander.input_queue.Insert(this);
    (new machine_work(&one_sided_sander, this))->Activate(); // ONE SIDED SANDER
    Passivate();

    // transport
    Wait(Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE));

    this->palette_done = 0;
    aligner.input_queue.Insert(this);
    (new machine_work(&aligner, this))->Activate(); // ALIGNER
    Passivate();

    // transport
    Wait(Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE));

    this->palette_done = 0;
    stretcher.input_queue.Insert(this);
    (new machine_work(&stretcher, this))->Activate(); // STRETCHER
    Passivate();

    // cleaning
    for (int i = this->palette_size; i > 0; i--) {
        Wait(10);
    }

    // transport
    Wait(Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE));

    this->palette_done = 0;
    double_sided_sander.input_queue.Insert(this);
    (new machine_work(&double_sided_sander, this))->Activate(); // DOUBLE SIDED SANDER
    Passivate();

    // quality control
    unsigned bad_pieces = 0;
    for (int i = this->palette_size; i > 0; i--) {
        double rand_n = Uniform(0, 100);

        if (rand_n <= BAD_PIECE_PERCENT) {
            bad_pieces++;
        }
    }
    Wait(PICE_REWORK_TIME * bad_pieces);

    this->palette_done = 0;
    oiling_machine.input_queue.Insert(this);
    (new machine_work(&oiling_machine, this))->Activate(); // OILING MACHINE
    Passivate();

    // transport
    Wait(Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE));

    this->palette_done = 0;

    int j = 0;
    int PACKAGE_SIZE = 50;
    for (int i = this->palette_size; i > 0; i -= PACKAGE_SIZE) {
        if (i < PACKAGE_SIZE) {
            PACKAGE_SIZE = i;
        }
        (new package_for_worker(this, j++, PACKAGE_SIZE))->Activate(Time + Normal(15, 1));
    }

    Passivate();

    this->order->add_pieces_done(this->palette_done);
    if (this->order->get_pieces_done() == this->order->get_order_size()) {
        this->order->Activate(); // activate the order process
    }

}

// ########## Simulation proccess for the maintenance of the machine ##########
maintenance::maintenance(machine *machine) : Process(1) {
    this->machine_to_maintain = machine;
}

void maintenance::Behavior() {
    Seize(*(this->machine_to_maintain));
    // if ((this->machine_to_maintain)->get_name() == "Pressing machine")
    //     cout << "START " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_HOUR << endl;

    Wait(Normal((this->machine_to_maintain)->get_maintenance_time(), (this->machine_to_maintain)->get_maintenance_time() * 0.1)); // 10% dispersion
    Release(*(this->machine_to_maintain));

    // if ((this->machine_to_maintain)->get_name() == "Pressing machine")
    //     cout << "END " << (this->machine_to_maintain)->get_name() << " time " << Time / SECONDS_IN_HOUR << endl;
}

// ########## Simulation proccess for the break of the worker ##########
break_worker::break_worker(worker *worker) : Process(1) {
    this->worker_to_break = worker;
}

void break_worker::Behavior() {
    Seize(*(this->worker_to_break));

    // if ((this->worker_to_break)->get_name_of_worker() == "Pressing machine worker" || (this->worker_to_break)->get_name_of_worker() == "One sided sander worker" || (this->worker_to_break)->get_name_of_worker() == "Aligner worker")
    //     cout << "START OF BREAK " << (this->worker_to_break)->get_name_of_worker() << " time " << Time / SECONDS_IN_HOUR << endl;

    Wait(Normal((this->worker_to_break)->get_break_time(), (this->worker_to_break)->get_break_time() * 0.1)); // 10% dispersion
    Release(*(this->worker_to_break));

    // if ((this->worker_to_break)->get_name_of_worker() == "Pressing machine worker" || (this->worker_to_break)->get_name_of_worker() == "One sided sander worker" || (this->worker_to_break)->get_name_of_worker() == "Aligner worker")
    //     cout << "END OF BREAK " << (this->worker_to_break)->get_name_of_worker() << " time " << Time / SECONDS_IN_HOUR << endl;
}

// ########### break_packaging_workers ###########
void break_packaging_workers::Behavior() {
    Enter(packing_workers, N_PACKING_WORKERS);
    Wait(Normal(10 * SECONDS_IN_MINUTE, 1 * SECONDS_IN_MINUTE));
    Leave(packing_workers, N_PACKING_WORKERS);
}

// ########## Simulation proccess for the order ##########
Order::Order() : Process() {
    this->order_size = static_cast<unsigned>(Uniform(ORDER_SIZE_MIN, ORDER_SIZE_MAX)); // random number of brake discs in the order
    this->amount_of_material = PIECE_MATERIAL_WEIGHT * this->order_size;               // amount of material needed for the order
    this->order_id = Order::order_count++;                                             // id of the order
}

void Order::Behavior() {
    // cout << "-----------------------------" << endl;
    // cout << "Order:" << endl;
    // cout << "\tOrder id: " << this->order_id << endl;
    // cout << "\tOrder came in: " << Time / SECONDS_IN_HOUR << endl;
    // cout << "\tOrder size: " << this->order_size << endl;
    // cout << "\tAmount of material: " << this->amount_of_material << endl;
    // cout << "-----------------------------" << endl;

    auto start_time = Time;

    Wait(SECONDS_IN_MINUTE);

    this->startTime = Time; // save the time of order

    if (warehouse.use_material(this->amount_of_material)) // chcek if there is enough material for the order
    {
        auto order_size_tmp = this->order_size;
        for (;;) {
            if (order_size_tmp > 1000) {
                (new palette(1000, this))->Activate(); // create new palette
                order_size_tmp -= 1000;
            } else {
                (new palette(order_size_tmp, this))->Activate();
                break;
            }
        }
        // machines[0]->input_queue.Insert(this); // insert the order into the queue of the first machine
    } else {
        // TODO: not enough material maybe the return is worng
        // cout << "ORDER N." << this->order_id << ": not enough material" << endl;
        return;
    }

    Passivate();

    cout << "-----------------------------" << endl;
    cout << "Order done:" << endl;
    cout << "\tOrder id: " << this->order_id << endl;
    cout << "\tOrder size: " << this->order_size << endl;
    cout << "\tOrder came in: " << start_time / SECONDS_IN_HOUR << endl;
    cout << "\tOrder done in: " << Time / SECONDS_IN_HOUR << endl;
    cout << "\tTime in production: " << (Time - start_time) / SECONDS_IN_HOUR << endl;
    cout << "-----------------------------" << endl;
    number_of_orders++;
    time_in_production_sum += (Time - start_time);
}

// ########## Process of supply of material ##########
void Supply::Behavior() {
    if (warehouse.add_material(MATERIAL_SUPPLY_WEIGHT)) {
        // cout << "SUPPLY: added 5000 kg of material" << endl;
    } else {
        return;
    }
}

// ########## Simulation proccess for machine work ##########

void machine_work::Behavior() {

    Seize(*(this->machine_to_work));
    if (this->machine_to_work->get_machine_id() != OILING_MACHINE) // oiling machine does not need worker
        Seize(*(this->machine_to_work->get_worker()));

    if (this->palette_in_machine->get_palette_id() == 0) {
        // cout << "===========================START===========================" << endl;
        // cout << "Machine: " << this->machine_to_work->get_name() << endl;
        // cout << "\tStart in time " << Time / SECONDS_IN_HOUR << endl;
        // cout << "\tPalette id: " << this->palette_in_machine->get_palette_id() << endl;
        // cout << "===========================================================" << endl;
    }

    Wait((this->machine_to_work)->get_preparation_time());

    switch (this->machine_to_work->get_machine_id()) {
    case PRESSING_MACHINE:
    case ONE_SIDED_SANDER:
    case ALIGNER:
    case DOUBLE_SIDED_SANDER:
    case OILING_MACHINE:
        for (int i = this->palette_in_machine->get_palette_size(); i > 0; i--) {
            Wait(this->machine_to_work->get_piece_production_time());
            this->palette_in_machine->increment_palette_done();
        }
        break;
    case STRETCHER:
        // redistribute paltte into packets by 15 pieces
        int PACKET_SIZE = 15;
        for (int i = this->palette_in_machine->get_palette_size(); i > 0; i -= PACKET_SIZE) {
            if (i < PACKET_SIZE) {
                PACKET_SIZE = i;
            }

            Wait(this->machine_to_work->get_piece_production_time());
            this->palette_in_machine->increment_palette_done(PACKET_SIZE);
        }
        break;
    }

    Release(*(this->machine_to_work));

    if (this->machine_to_work->get_machine_id() != OILING_MACHINE)
        Release(*(this->machine_to_work->get_worker()));

    if (this->palette_in_machine->get_palette_id() == 0) {
        // cout << "=============================END=========================" << endl;
        // cout << "Machine: " << this->machine_to_work->get_name() << endl;
        // cout << "\tDone in time " << Time / SECONDS_IN_HOUR << endl;
        // cout << "\tPalette id: " << this->palette_in_machine->get_palette_id() << endl;
        // cout << "\tBrake discs done: " << this->palette_in_machine->get_palette_done() << endl;
        // cout << "=========================================================" << endl;
    }

    if (!this->machine_to_work->input_queue.Empty())
        this->machine_to_work->input_queue.GetFirst()->Activate();
}

// ########## Simulation proccess for packing ##########
void package_for_worker::Behavior() {
    // cout << "===========================START===========================" << endl;
    // cout << "\tStart in time " << Time / SECONDS_IN_HOUR << endl;
    // cout << "\tPalette id: " << this->palette_to_pack->get_palette_id() << endl;
    // cout << "\tPackage id: " << this->package_id << endl;
    // cout << "===========================================================" << endl;

    Enter(packing_workers, 1);
    Wait(3 * SECONDS_IN_MINUTE);
    Leave(packing_workers, 1);

    // cout << "=============================END=========================" << endl;
    // cout << "\tDone in time " << Time / SECONDS_IN_HOUR << endl;
    // cout << "\tPalette id: " << this->palette_to_pack->get_palette_id() << endl;
    // cout << "\tPackage id: " << this->package_id << endl;
    // cout << "=========================================================" << endl;

    this->palette_to_pack->increment_palette_done(this->size);

    if (this->palette_to_pack->get_palette_done() == this->palette_to_pack->get_palette_size()) {
        this->palette_to_pack->Activate(); // activate the palette process
    }
}

//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
void maintenance_event::Behavior() {
    Activate(Time + Normal(SECONDS_IN_HOUR * 8, 0)); // schedule the next maintenance event after 8 hours

    for (int i = 0; i < 6; i++) {
        (new maintenance(machines[i]))->Activate(); // activate the maintenance process for each machine
    }
}

// ########## Event for the break ##########
void break_event::Behavior() {
    Activate(Time + Normal(SECONDS_IN_HOUR * 2, SECONDS_IN_MINUTE * 3)); // schedule the next break event after 2 hours and with some +- 5 minute dispersion

    for (int i = 0; i < 6; i++) {
        (new break_worker(workers[i]))->Activate(); // activate the break process for each worker
    }

    (new break_packaging_workers)->Activate(); // activate the break process for packaging workers

    // cout << "Break event time: " << Time / SECONDS_IN_MINUTE << endl;
}

// ########## Generator for new orders ##########
void order_event::Behavior() {
    (new Order)->Activate();
    Activate(Time + Exponential(SECONDS_IN_HOUR * 8)); // order every 8 hours (exponential)
}

// ########## Generator for supplies ##########
void supply_event::Behavior() {
    (new Supply)->Activate();
    Activate(Time + Normal(SECONDS_IN_DAY, SECONDS_IN_HOUR * 2)); // supply every 24 hours (normal)
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

void fill_worker_array() {
    workers[0] = &pressing_machine_worker;
    workers[1] = &one_sided_sander_worker;
    workers[2] = &aligner_worker;
    workers[3] = &stretcher_worker;
    workers[4] = &double_sided_sander_worker;
    workers[5] = &oiling_machine_worker;
}

void help(const char *prog_name) {
    cout << prog_name << " program implements the SHO of the engineering company." << endl;
    cout << "Subject: IMS" << endl;
    cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    cout << "          Adam Pap (xpapad11) " << endl;
    cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

int main(int argc, char *argv[]) {

    // parameter handling
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        help(argv[0]);
        return 0;
    }

    fill_machine_array();
    fill_worker_array();

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, SECONDS_IN_DAY * 1000); // time of simulation
    (new order_event)->Activate();
    (new supply_event)->Activate();
    (new maintenance_event)->Activate(Time + SECONDS_IN_HOUR * 8); // first maintenance after 8 hours not at the start of the simulation
    (new break_event)->Activate(Time + SECONDS_IN_HOUR * 2);       // first break after 2 hours not at the start of the simulation
    Run();

    // print out statistics TODO: ...

    // cout << "Warehouse cap: " << warehouse.get_current() << endl;

    cout << "------------ STATISTICS ------------" << endl;
    cout << "Number of orders: " << number_of_orders << endl;
    cout << "Average time in production (HOURS): " << time_in_production_sum / number_of_orders / SECONDS_IN_HOUR << " hours" << endl;

    return ErrCode(SUCCESS);
}

// ended at Many fixes + machine_maintenance & machine classes added. Removed pre…