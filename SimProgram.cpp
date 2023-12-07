#include "SimProgram.hpp"

using namespace std;

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- GLOBALS ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------ //

// STATISTICS
unsigned orders_done = 0;
double time_in_production_sum = 0;
double amount_of_material_done = 0;
unsigned not_enough_material = 0;
unsigned amount_of_material_sent_back = 0;

unsigned palettes_done = 0;
double spent_in_rework_all = 0;
unsigned pieces_to_rework_all = 0;
double spent_in_quality_control = 0;
double spent_in_packing = 0;

Stat palette_time("Palette (2000 pcs) time in production (HOURS)");

// ########################################### GLOBAL WAREHOUSE FOR MATERIAL ###########################################
material_warehouse warehouse("Material warehouse", (float)MATERIAL_WAREHOUSE_CAPACITY, (float)INITIAL_MATERIAL_WAREHOUSE_WEIGHT);

// ########################################### WORKERS ###########################################
worker pressing_machine_worker(N_PRESSING, WORKER_BREAK, "Pressing machine worker");
worker one_sided_sander_worker(N_ONE_SIDED_SANDER, WORKER_BREAK, "One sided sander worker");
worker aligner_worker(N_ALIGNER, WORKER_BREAK, "Aligner worker");
worker stretcher_worker(N_STRETCHER, WORKER_BREAK, "Stretcher worker");
worker double_sided_sander_worker(N_DOUBLE_SIDED_SANDER, WORKER_BREAK, "Double sided sander worker");
worker oiling_machine_worker(N_OILING, WORKER_BREAK, "Oiling machine worker");
worker packing_workers(N_PACKING_WORKERS, WORKER_BREAK, "Packing workers");
worker quality_controllers(N_QUALITY_CONTROLLERS, WORKER_BREAK, "Quality controllers");
worker *workers[7];

// ########################################### MACHINES ###########################################
machine pressing_machine(N_PRESSING, maintenance_time[0][0], 60 * SECONDS_IN_MINUTE, 1, "Pressing machine", &pressing_machine_worker, PRESSING_MACHINE);
machine one_sided_sander(N_ONE_SIDED_SANDER, maintenance_time[1][0], 10 * SECONDS_IN_MINUTE, 10, "One sided sander", &one_sided_sander_worker, ONE_SIDED_SANDER);
machine aligner(N_ALIGNER, maintenance_time[2][0], 10 * SECONDS_IN_MINUTE, 10, "Aligner", &aligner_worker, ALIGNER);
machine stretcher(N_STRETCHER, maintenance_time[3][0], 50 * SECONDS_IN_MINUTE, 2.5 * SECONDS_IN_MINUTE, "Stretcher", &stretcher_worker, STRETCHER);
machine double_sided_sander(N_DOUBLE_SIDED_SANDER, maintenance_time[4][0], 10 * SECONDS_IN_MINUTE, 10, "Double sided sander", &double_sided_sander_worker, DOUBLE_SIDED_SANDER);
machine oiling_machine(N_OILING, maintenance_time[5][0], 0, 2, "Oiling machine", &oiling_machine_worker, OILING_MACHINE);
machine *machines[6];

// ------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- CLASSES ----------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- FACILITIES --------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########################################### Material warehouse ###########################################
material_warehouse::material_warehouse(string desc, float max_capacity, float current_capacity) : Facility() {
    (void)desc;
    this->max = max_capacity;
    this->current = current_capacity;
}

float material_warehouse::get_current() { return this->current; }

bool material_warehouse::add_material(float amount) {
    if (this->current + amount > this->max) {
        amount_of_material_sent_back += this->current + amount - this->max;
        this->current = this->max;
        return false;
    }

    this->current += amount; // increase the capacity of the warehouse by the amount of material
    return true;
}

bool material_warehouse::use_material(float amount) {
    if (amount > this->current) // chceck if the amount is not bigger than the capacity
    {
        return false;
    }
    this->current -= amount; // decrease the capacity of the warehouse by the amount of material needed for production

    return true;
}

// ------------------------------------------------------------------------------------------------------- //
//----------------------------------------------- PROCESSES ---------------------------------------------- //
// ------------------------------------------------------------------------------------------------------- //

// ########################################### Simulation proccess for the production of the palettes of brake discs ###########################################

void palette::Behavior() {
    // cout << "Palette id: " << this->palette_id << endl;
    // cout << "\tPalette size: " << this->palette_size << endl;
    // cout << "\tStart time: " << this->startTime / SECONDS_IN_MINUTE << endl;

    this->startTime = Time;
    (new machine_work(&pressing_machine, this))->Activate(); // PRESSING MACHINE
    Passivate();

    // transport
    double rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(&one_sided_sander, this))->Activate(); // ONE SIDED SANDER
    Passivate();

    // transport
    rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(&aligner, this))->Activate(); // ALIGNER
    Passivate();

    // transport
    rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(&stretcher, this))->Activate(); // STRETCHER
    Passivate();

    // cleaning
    for (int i = this->palette_size; i > 0; i--) {
        Wait(10);
    }

    // transport
    rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(&double_sided_sander, this))->Activate(); // DOUBLE SIDED SANDER
    Passivate();

    // quality control
    double gc_start_time = Time;
    this->palette_done = 0;
    for (int i = this->palette_size; i > 0; i--) {
        (new quality_control(this))->Activate();
    }
    Passivate();
    double gc_end_time = Time;
    if (this->bad_pieces > 0)
        Wait(PIECE_REWORK_TIME * this->bad_pieces);

    this->palette_done = 0;
    (new machine_work(&oiling_machine, this))->Activate(); // OILING MACHINE
    Passivate();

    // packing
    double packing_start_time = Time;
    this->palette_done = 0;
    int package_id = 0; // package id
    int PACKAGE_SIZE = 50;
    for (int i = this->palette_size; i > 0; i -= PACKAGE_SIZE) {
        if (i < PACKAGE_SIZE) {
            PACKAGE_SIZE = i;
        }
        rnd_i = Normal(15, 1);
        if (rnd_i <= 0)
            (new package_for_worker(this, package_id++, PACKAGE_SIZE))->Activate(Time + 15);
        else
            (new package_for_worker(this, package_id++, PACKAGE_SIZE))->Activate(Time + rnd_i);
    }
    Passivate();
    double packing_end_time = Time;

    if (this->palette_size == PALETTE_PIECES) // only count duration for the palette of 2000 pieces
        palette_time((Time - this->startTime) / SECONDS_IN_HOUR);

    spent_in_rework_all += PIECE_REWORK_TIME * this->bad_pieces;
    pieces_to_rework_all += this->bad_pieces;
    spent_in_quality_control += (gc_end_time - gc_start_time);
    spent_in_packing += (packing_end_time - packing_start_time);
    palettes_done++;

    this->order->add_pieces_done(this->palette_done);
    if (this->order->get_pieces_done() == this->order->get_order_size()) {
        this->order->Activate(); // activate the order process
    }
}

// ########################################### Simulation proccess for the maintenance of the machine ###########################################
maintenance::maintenance(machine *machine) : Process(1) {
    this->machine_to_maintain = machine;
}

void maintenance::Behavior() {
    Enter(*(this->machine_to_maintain), this->machine_to_maintain->get_store_capacity());

    double wait_t = Normal((this->machine_to_maintain)->get_maintenance_time(), (this->machine_to_maintain)->get_maintenance_time() * 0.1); // 10% dispersion
    if (wait_t > 0)
        Wait(wait_t);
    Leave(*(this->machine_to_maintain), this->machine_to_maintain->get_store_capacity());
}

// ########################################### Simulation proccess for the break of the worker ###########################################
void break_worker::Behavior() {
    if (this->worker_to_break->get_name_of_worker() != "Oiling machine worker")
        this->worker_to_break->start_break();

    Enter(*(this->worker_to_break), this->worker_to_break->get_n_workers());

    double wait_t = Normal((this->worker_to_break)->get_break_time(), (this->worker_to_break)->get_break_time() * 0.1); // 10% dispersion
    if (wait_t > 0)
        Wait(wait_t);
    Leave(*(this->worker_to_break), this->worker_to_break->get_n_workers());

    if (this->worker_to_break->get_name_of_worker() != "Oiling machine worker") {
        this->worker_to_break->end_break();
        auto tasks = this->worker_to_break->get_current_tasks();

        for (auto task : tasks) {
            if (task != nullptr)
                task->Activate();
        }
    }
}

// ########################################### Simulation proccess for the order ###########################################
Order::Order() : Process() {
    this->order_size = static_cast<unsigned>(Uniform(ORDER_SIZE_MIN, ORDER_SIZE_MAX)); // random number of brake discs in the order
    this->amount_of_material = PIECE_MATERIAL_WEIGHT * this->order_size;               // amount of material needed for the order
    this->order_id = Order::order_count++;                                             // id of the order
}

void Order::Behavior() {
    auto start_time = Time;

    Wait(SECONDS_IN_MINUTE);

    this->startTime = Time; // save the time of order

    if (warehouse.use_material(this->amount_of_material)) // check if there is enough material for the order
    {
        auto order_size_tmp = this->order_size;
        for (;;) {
            if (order_size_tmp > PALETTE_PIECES) {
                (new palette(PALETTE_PIECES, this))->Activate(); // create new palette
                order_size_tmp -= PALETTE_PIECES;
            } else {
                (new palette(order_size_tmp, this))->Activate();
                break;
            }
        }
    } else {
        // TODO: not enough material maybe the return is worng
        not_enough_material++;
        return;
    }

    Passivate();

    orders_done++;
    time_in_production_sum += (Time - start_time);
    amount_of_material_done += this->amount_of_material;
}

// ########################################### Process of supply of material ###########################################
void Supply::Behavior() {
    warehouse.add_material(MATERIAL_SUPPLY_WEIGHT);
}

// ########################################### Simulation proccess for machine work ###########################################

void machine_work::Behavior() {
    this->machine_to_work->pallets_waiting.Insert(this->palette_in_machine);
    Enter(*(this->machine_to_work), 1);
    if (!this->machine_to_work->pallets_waiting.Empty())
        this->machine_to_work->pallets_waiting.GetFirst();

    double start_time = Time;

    if (this->machine_to_work->get_machine_id() != OILING_MACHINE) // oiling machine does not need worker
    {
        Enter(*(this->machine_to_work->get_worker()), 1);
        this->machine_to_work->get_worker()->start_task(this);
    }

    for (int i = this->machine_to_work->get_preparation_time(); i > 0; i--) {
        if (this->machine_to_work->get_worker()->is_break_time()) {
            Leave(*(this->machine_to_work->get_worker()), 1);
            Passivate();
            Enter(*(this->machine_to_work->get_worker()), 1);
        }
        Wait(1);
    }

    switch (this->machine_to_work->get_machine_id()) {
    case PRESSING_MACHINE:
    case ONE_SIDED_SANDER:
    case ALIGNER:
    case DOUBLE_SIDED_SANDER:
    case OILING_MACHINE:
        for (int i = this->palette_in_machine->get_palette_size(); i > 0; i--) {

            if (this->machine_to_work->get_worker()->is_break_time() && this->machine_to_work->get_machine_id() != OILING_MACHINE) {
                Leave(*(this->machine_to_work->get_worker()), 1);
                Passivate();
                Enter(*(this->machine_to_work->get_worker()), 1);
            }

            Wait(this->machine_to_work->get_piece_production_time());
            this->palette_in_machine->increment_palette_done();
        }
        break;
    case STRETCHER:
        // redistribute palette into packets by 15 pieces
        int PACKET_SIZE = 15;

        for (int i = this->palette_in_machine->get_palette_size(); i > 0; i -= PACKET_SIZE) {
            if (i < PACKET_SIZE) {
                PACKET_SIZE = i;
            }

            if (this->machine_to_work->get_worker()->is_break_time()) {
                Leave(*(this->machine_to_work->get_worker()), 1);
                Passivate();
                Enter(*(this->machine_to_work->get_worker()), 1);
            }

            Wait(this->machine_to_work->get_piece_production_time());
            this->palette_in_machine->increment_palette_done(PACKET_SIZE);
        }
        break;
    }

    if (this->machine_to_work->get_machine_id() != OILING_MACHINE) // oiling machine does not need worker
    {
        this->machine_to_work->get_worker()->remove_task(this);
        Leave(*(this->machine_to_work->get_worker()), 1);
    }   

    Leave(*(this->machine_to_work), 1);

    this->machine_to_work->pallets_time_all += (Time - start_time);
    this->machine_to_work->pallets_done++;

    this->palette_in_machine->Activate(); // activate the palette process
}

// ########## Simulation proccess for packing ##########
void package_for_worker::Behavior() {

    Enter(packing_workers, 1);
    Wait(3 * SECONDS_IN_MINUTE);
    Leave(packing_workers, 1);

    this->palette_to_pack->increment_palette_done(this->size);

    if (this->palette_to_pack->get_palette_done() == this->palette_to_pack->get_palette_size()) {
        this->palette_to_pack->Activate(); // activate the palette process
    }
}

// ########### Quality control ###########
void quality_control::Behavior() {
    Enter(quality_controllers, 1);
    double rand_n = Uniform(0, 100);

    if (rand_n <= BAD_PIECE_PERCENT) {
        this->palette_to_check->increment_bad_pieces();
    }
    Wait(PIECE_CONTROL_TIME);
    Leave(quality_controllers, 1);

    this->palette_to_check->increment_palette_done();

    if (this->palette_to_check->get_palette_done() == this->palette_to_check->get_palette_size()) {
        this->palette_to_check->Activate(); // activate the palette process
    }
}

//----------------------------------------------- EVENTS -----------------------------------------------

// ########################################### Event for the maintenance ###########################################
void maintenance_event::Behavior() {
    Activate(Time + Normal(SECONDS_IN_HOUR * 8, 0)); // schedule the next maintenance event after 8 hours

    for (int i = 0; i < 6; i++) {
        (new maintenance(machines[i]))->Activate();
    }
}

// ########################################### Event for the break ###########################################
void break_event::Behavior() {
    Activate(Time + Normal(SECONDS_IN_HOUR * 2, 0)); // schedule the next break event after 2 hours

    for (int i = 0; i < 7; i++) {
        (new break_worker(workers[i]))->Activate();
    }
}

// ########################################### Generator for new orders ###########################################
void order_event::Behavior() {
    double rnd_i = Exponential(SECONDS_IN_HOUR * 42); // order every 42 hours (exponential)

    if (rnd_i > 0)
        Activate(Time + rnd_i);
    else
        Activate(Time + SECONDS_IN_HOUR * 42);

    (new Order)->Activate();
}

// ########################################### Generator for supplies ###########################################
void supply_event::Behavior() {
    double rnd_i = Normal(SECONDS_IN_DAY * 14, SECONDS_IN_HOUR * 2); // supply every 14 days (normal)
    if (rnd_i > 0)
        Activate(Time + rnd_i);
    else
        Activate(Time + SECONDS_IN_DAY * 14);

    (new Supply)->Activate();
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
    workers[6] = &packing_workers;
}

void help(const char *prog_name) {
    cout << prog_name << " program implements the SHO of the engineering company. Default simulation run is 365 days." << endl;
    cout << "Usage: " << prog_name << " [--help] [simulation_duration]" << endl;
    cout << "\tsimulation_duration - duration of the simulation in days" << endl << endl;
    cout << "Subject: IMS" << endl;
    cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    cout << "          Adam Pap (xpapad11) " << endl;
    cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

void print_stats() {
    cout << "------------------------------------------------------------" << endl;
    cout << "|                    FACTORY SIMULATION                    |" << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "Simulation time: " << SIMULATION_TIME / SECONDS_IN_DAY << " days" << endl;
    cout << "Number of orders done: " << orders_done << endl;
    cout << "Orders refused (not enough material): " << not_enough_material << endl;
    cout << "Average order time in production (HOURS): " << (orders_done ? time_in_production_sum / orders_done / SECONDS_IN_HOUR : 0) << " hours" << endl;
    cout << "Average amount of material of done orders: " << (orders_done ? amount_of_material_done / orders_done : 0) << " kg" << endl;
    cout << "Amount of material sent back (full warehouse): " << amount_of_material_sent_back << " kg" << endl;

    cout << endl;

    palette_time.Output();

    cout << endl << endl;

    for(int i = 0; i < 6; i++)
    {
        cout << machines[i]->get_name()<<": "<< endl;
        cout << "------------------------------------------------------------"<<endl;
        cout << "Number of machines: " << machines[i]->get_store_capacity() << endl;
        cout << "Average time spent in machine: " << machines[i]->get_average_pallet_time() / SECONDS_IN_HOUR << " hours" << endl;
        cout << "Pallets done: " << machines[i]->pallets_done << endl;
        cout << "Waiting queue: " << endl;
        machines[i]->pallets_waiting.Output();
        cout << endl << endl;
    }

    cout << "Quality control: " << endl;
    cout << "------------------------------------------------------------"<<endl;
    cout << "Number of quality controllers: " << quality_controllers.get_n_workers() << endl;
    cout << "Average number of bad pieces: " << (palettes_done ? (double)pieces_to_rework_all / palettes_done : 0) << endl;
    cout << "Average time spent in rework: " << (palettes_done ? spent_in_rework_all / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;
    cout << "Average time spent in quality control: " << (palettes_done ? spent_in_quality_control / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;

    cout << endl << endl;

    cout << "Packing: " << endl;
    cout << "------------------------------------------------------------"<<endl;
    cout << "Number of packing workers: " << packing_workers.get_n_workers() << endl;
    cout << "Packing queue: " << endl;
    cout << "Average time spent in packing: " << (palettes_done ? spent_in_packing / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;

    cout << endl;
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0) {
            help(argv[0]);
            return 0;
        } else {
            double tmp = SIMULATION_TIME;
            try {
                SIMULATION_TIME = stod(argv[1]) * SECONDS_IN_DAY;
            } catch (const std::exception &e) {
                SIMULATION_TIME = tmp;
            }
        }
    }

    fill_machine_array();
    fill_worker_array();

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, SIMULATION_TIME); // time of simulation
    (new order_event)->Activate();
    (new supply_event)->Activate();
    (new maintenance_event)->Activate(Time + SECONDS_IN_HOUR * 8); // first maintenance after 8 hours not at the start of the simulation
    (new break_event)->Activate(Time + SECONDS_IN_HOUR * 2);       // first break after 2 hours not at the start of the simulation
    Run();

    print_stats();

    return ErrCode(SUCCESS);
}
