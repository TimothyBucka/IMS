#include "SimProgram.hpp"

using namespace std;

// ------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------- GLOBALS ------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------- //

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
worker *workers[8];

// ########################################### MACHINES ###########################################
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
    this->startTime = Time;
    (new machine_work(machines[PRESSING_MACHINE], this))->Activate(); // PRESSING MACHINE
    Passivate();

    // transport
    double rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(machines[ONE_SIDED_SANDER], this))->Activate(); // ONE SIDED SANDER
    Passivate();

    // transport
    rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(machines[ALIGNER], this))->Activate(); // ALIGNER
    Passivate();

    // transport
    rnd_i = Normal(15 * SECONDS_IN_MINUTE, 4 * SECONDS_IN_MINUTE);
    if (rnd_i > 0)
        Wait(rnd_i);

    this->palette_done = 0;
    (new machine_work(machines[STRETCHER], this))->Activate(); // STRETCHER
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
    (new machine_work(machines[DOUBLE_SIDED_SANDER], this))->Activate(); // DOUBLE SIDED SANDER
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
    (new machine_work(machines[OILING_MACHINE], this))->Activate(); // OILING MACHINE
    Passivate();

    // packing
    double packing_start_time = Time;
    this->palette_done = 0;
    int package_id = 0; // package id
    int PACKAGE_SIZE = 40;
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

    if (this->palette_size == PALETTE_PIECES) {
        cout << "Palette:" << (Time - this->startTime) / SECONDS_IN_HOUR << endl;
        palette_time((Time - this->startTime) / SECONDS_IN_HOUR); // only count duration for the palette of 2000 pieces
    }

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
        not_enough_material++;
        return;
    }

    Passivate();

    cout<<"Order:"<<(Time - start_time)/SECONDS_IN_HOUR<<","<<this->order_size<<endl;

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
    default:
        break;
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

    Enter(*workers[PACKING], 1);
    Wait(3 * SECONDS_IN_MINUTE);
    Leave(*workers[PACKING], 1);

    this->palette_to_pack->increment_palette_done(this->size);

    if (this->palette_to_pack->get_palette_done() == this->palette_to_pack->get_palette_size()) {
        this->palette_to_pack->Activate(); // activate the palette process
    }
}

// ########### Quality control ###########
void quality_control::Behavior() {
    Enter(*workers[QUALITY_CONTROL], 1);
    double rand_n = Uniform(0, 100);

    if (rand_n <= BAD_PIECE_PERCENT) {
        this->palette_to_check->increment_bad_pieces();
    }
    Wait(PIECE_CONTROL_TIME);
    Leave(*workers[QUALITY_CONTROL], 1);

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

    for (int i = 0; i < 8; i++) {
        (new break_worker(workers[i]))->Activate();
    }
}

// ########################################### Generator for new orders ###########################################
void order_event::Behavior() {
    double rnd_i = Exponential(SECONDS_IN_HOUR * 8); // order every 8 hours (exponential)

    if (rnd_i > 0)
        Activate(Time + rnd_i);
    else
        Activate(Time + SECONDS_IN_HOUR * 8);

    (new Order)->Activate();
}

// ########################################### Generator for supplies ###########################################
void supply_event::Behavior() {
    double rnd_i = Normal(SECONDS_IN_DAY * 2, SECONDS_IN_HOUR * 2); // supply every 2 days (normal)
    if (rnd_i > 0)
        Activate(Time + rnd_i);
    else
        Activate(Time + SECONDS_IN_DAY * 2);

    (new Supply)->Activate();
}

// --------------------------------------------------------------------------------------------------- //
// ----------------------------------------------- FUNCS --------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //

void help(const char *prog_name) {
    cout << prog_name << " program implements the SHO of the engineering company. Default simulation run is 365 days." << endl;
    cout << "Usage: " << prog_name << " [options]" << endl;
    cout << "\t--help, -h              : prints help" <<endl;
    cout << "\t<simulation_duration>   : simulation duration in days" << endl;
    cout << "\t--duration, -d <days>   : simulation duration in days" << endl;
    cout << "\t--experiment, -e <number 0-2> : run given experiment (with machine configuration)" << endl;
    cout<< "\t\t0:" << endl;
    cout << "\t\t\tPressing machine machines: 2" << endl;
    cout << "\t\t\tOne sided sander machines: 3" << endl;
    cout << "\t\t\tAligner machines: 2" << endl;
    cout << "\t\t\tStretcher machines: 3" << endl;
    cout << "\t\t\tDouble sided sander machines: 2" << endl;
    cout << "\t\t\tOiling machines: 1" << endl;
    cout << "\t\t\tPacking workers: 10" << endl;
    cout << "\t\t\tQuality controllers: 10" << endl;
    cout<< "\t\t1:" << endl;
    cout << "\t\t\tPressing machine machines: 2" << endl;
    cout << "\t\t\tOne sided sander machines: 4" << endl;
    cout << "\t\t\tAligner machines: 3" << endl;
    cout << "\t\t\tStretcher machines: 3" << endl;
    cout << "\t\t\tDouble sided sander machines: 3" << endl;
    cout << "\t\t\tOiling machines: 1" << endl;
    cout << "\t\t\tPacking workers: 10" << endl;
    cout << "\t\t\tQuality controllers: 10" << endl;
    cout<< "\t\t2:" << endl;
    cout << "\t\t\tPressing machine machines: 2" << endl;
    cout << "\t\t\tOne sided sander machines: 5" << endl;
    cout << "\t\t\tAligner machines: 4" << endl;
    cout << "\t\t\tStretcher machines: 4" << endl;
    cout << "\t\t\tDouble sided sander machines: 4" << endl;
    cout << "\t\t\tOiling machines: 2" << endl;
    cout << "\t\t\tPacking workers: 10" << endl;
    cout << "\t\t\tQuality controllers: 10" << endl;
    cout << "\t--press <number>        : number of pressing machines" << endl;
    cout << "\t--1_sided <number>    : number of one sided sander machines" << endl;
    cout << "\t--align <number>      : number of aligner machines" << endl;
    cout << "\t--stretch <number>    : number of stretcher machines" << endl;
    cout << "\t--2_sided <number> : number of double sided sander machines" << endl;
    cout << "\t--oil <number>       : number of oiling machines" << endl;
    cout << "\t--pack <number>      : number of packing workers" << endl;
    cout << "\t--quality <number>   : number of quality controllers" << endl;
    cout << endl;
    cout << "Subject: IMS" << endl;
    cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    cout << "          Adam Pap (xpapad11) " << endl;
    cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

void print_stats(ProgramOptions &options) {
    cout << "------------------------------------------------------------" << endl;
    cout << "|                    FACTORY SIMULATION                    |" << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "Simulation time: " << options.SIMULATION_DURATION / SECONDS_IN_DAY << " days" << endl;
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
    cout << "Number of quality controllers: " << workers[QUALITY_CONTROL]->get_n_workers() << endl;
    cout << "Average number of bad pieces: " << (palettes_done ? (double)pieces_to_rework_all / palettes_done : 0) << endl;
    cout << "Average time spent in rework: " << (palettes_done ? spent_in_rework_all / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;
    cout << "Average time spent in quality control: " << (palettes_done ? spent_in_quality_control / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;

    cout << endl << endl;

    cout << "Packing: " << endl;
    cout << "------------------------------------------------------------"<<endl;
    cout << "Number of packing workers: " << workers[PACKING]->get_n_workers() << endl;
    cout << "Average time spent in packing: " << (palettes_done ? spent_in_packing / palettes_done / SECONDS_IN_MINUTE : 0) << " minutes" << endl;

    cout << endl;
}

bool parse_args(int argc, char *argv[], ProgramOptions &options) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.help = true;
            return true;

        } else if (arg == "--duration" || arg == "-d") {
            if (i + 1 < argc) {
                try {
                    options.SIMULATION_DURATION = stod(argv[++i]) * SECONDS_IN_DAY;
                } catch (exception &e) {
                    cerr << "Error: --duration option requires a numeric argument.\n";
                    return false;
                }
            } else {
                cerr << "Error: --duration option requires a numeric argument.\n";
                return false;
            }

        } else if (arg == "--experiment" || arg == "-e") {
            if (i + 1 < argc) {
                try {
                    options.EXPERIMENT = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --experiment option requires a numeric argument in range 0-2.\n";
                    return false;
                }

                if (options.EXPERIMENT > 2) {
                    cerr << "Error: --experiment option requires a numeric argument in range 0-2.\n";
                    return false;
                }
            } else {
                cerr << "Error: --run option requires a numeric argument in range 0-2.\n";
                return false;
            }

        } else if (arg == "--press") {
            if (i + 1 < argc) {
                try {
                    options.N_PRESSING = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --press option requires a numeric argument.\n";
                    return false;
                }
                options.press = true;
            } else {
                cerr << "Error: --press option requires a numeric argument.\n";
                return false;
            }

        } else if (arg == "--1_sided") {
            if (i + 1 < argc) {
                try {
                    options.N_ONE_SIDED_SANDER = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --1_sided option requires a numeric argument.\n";
                    return false;
                }
                options.one_sided = true;
            } else {
                cerr << "Error: --1_sided option requires a numeric argument.\n";
                return false;
            }

        } else if (arg == "--align") {
            if (i + 1 < argc) {
                try {
                    options.N_ALIGNER = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --align option requires a numeric argument.\n";
                    return false;
                }
                options.align = true;
            } else {
                cerr << "Error: --align option requires a numeric argument.\n";
                return false;
            }
        
        } else if (arg == "--stretch") {
            if (i + 1 < argc) {
                try {
                    options.N_STRETCHER = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --stretch option requires a numeric argument.\n";
                    return false;
                }
                options.stretch = true;
            } else {
                cerr << "Error: --stretch option requires a numeric argument.\n";
                return false;
            }
    
        } else if (arg == "--2_sided") {
            if (i + 1 < argc) {
                try {
                    options.N_DOUBLE_SIDED_SANDER = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --2_sided option requires a numeric argument.\n";
                    return false;
                }
                options.double_sided = true;
            } else {
                cerr << "Error: --2_sided option requires a numeric argument.\n";
                return false;
            }
        
        } else if (arg == "--oil") {
            if (i + 1 < argc) {
                try {
                    options.N_OILING = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --oil option requires a numeric argument.\n";
                    return false;
                }
                options.oil = true;
            } else {
                cerr << "Error: --oil option requires a numeric argument.\n";
                return false;
            }
        
        } else if (arg == "--pack") {
            if (i + 1 < argc) {
                try {
                    options.N_PACKING_WORKERS = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --pack option requires a numeric argument.\n";
                    return false;
                }
                options.pack = true;
            } else {
                cerr << "Error: --pack option requires a numeric argument.\n";
                return false;
            }
        
        } else if (arg == "--quality") {
            if (i + 1 < argc) {
                try {
                    options.N_QUALITY_CONTROLLERS = stoul(argv[++i]);
                } catch (exception &e) {
                    cerr << "Error: --quality option requires a numeric argument.\n";
                    return false;
                }
                options.quality = true;
            } else {
                cerr << "Error: --quality option requires a numeric argument.\n";
                return false;
            }
        
        } else {
            try {
                options.SIMULATION_DURATION = stod(arg) * SECONDS_IN_DAY;
            } catch (exception &e) {
                cerr << "Error: unknown option " << arg << endl;
                return false;
            }
        }
    }

    switch (options.EXPERIMENT)
    {
    case 1:
        options.N_PRESSING = options.press ? options.N_PRESSING : 2;
        options.N_ONE_SIDED_SANDER = options.one_sided ? options.N_ONE_SIDED_SANDER : 4;
        options.N_ALIGNER = options.align ? options.N_ALIGNER : 3;
        options.N_STRETCHER = options.stretch ? options.N_STRETCHER : 3;
        options.N_DOUBLE_SIDED_SANDER = options.double_sided ? options.N_DOUBLE_SIDED_SANDER : 3;
        options.N_OILING = options.oil ? options.N_OILING : 1;
        options.N_PACKING_WORKERS = options.pack ? options.N_PACKING_WORKERS : 10;
        options.N_QUALITY_CONTROLLERS = options.quality ? options.N_QUALITY_CONTROLLERS : 10;
        break;

    case 2:
        options.N_PRESSING = options.press ? options.N_PRESSING : 2;
        options.N_ONE_SIDED_SANDER = options.one_sided ? options.N_ONE_SIDED_SANDER : 5;
        options.N_ALIGNER = options.align ? options.N_ALIGNER : 4;
        options.N_STRETCHER = options.stretch ? options.N_STRETCHER : 4;
        options.N_DOUBLE_SIDED_SANDER = options.double_sided ? options.N_DOUBLE_SIDED_SANDER : 4;
        options.N_OILING = options.oil ? options.N_OILING : 2;
        options.N_PACKING_WORKERS = options.pack ? options.N_PACKING_WORKERS : 10;
        options.N_QUALITY_CONTROLLERS = options.quality ? options.N_QUALITY_CONTROLLERS : 10;
        break;

    default:
        break;
    }

    return true;
}

int main(int argc, char *argv[]) {
    ProgramOptions options;

    if (!parse_args(argc, argv, options)) {
        return ErrCode(FAIL);
    }
    if (options.help) {
        help(argv[0]);
        return ErrCode(SUCCESS);
    }

    // fill worker array
    worker pressing_machine_worker(options.N_PRESSING, WORKER_BREAK, "Pressing machine worker");
    workers[PRESSING_MACHINE] = &pressing_machine_worker;
    worker one_sided_sander_worker(options.N_ONE_SIDED_SANDER, WORKER_BREAK, "One sided sander worker");
    workers[ONE_SIDED_SANDER] = &one_sided_sander_worker;
    worker aligner_worker(options.N_ALIGNER, WORKER_BREAK, "Aligner worker");
    workers[ALIGNER] = &aligner_worker;
    worker stretcher_worker(options.N_STRETCHER, WORKER_BREAK, "Stretcher worker");
    workers[STRETCHER] = &stretcher_worker;
    worker double_sided_sander_worker(options.N_DOUBLE_SIDED_SANDER, WORKER_BREAK, "Double sided sander worker");
    workers[DOUBLE_SIDED_SANDER] = &double_sided_sander_worker;
    worker oiling_machine_worker(options.N_OILING, WORKER_BREAK, "Oiling machine worker");
    workers[OILING_MACHINE] = &oiling_machine_worker;
    worker packing_workers(options.N_PACKING_WORKERS, WORKER_BREAK, "Packing workers");
    workers[PACKING] = &packing_workers;
    worker quality_controllers(options.N_QUALITY_CONTROLLERS, WORKER_BREAK, "Quality controllers");
    workers[QUALITY_CONTROL] = &quality_controllers;

    // fill machine array
    machine pressing_machine(options.N_PRESSING, maintenance_time[0][0], 60 * SECONDS_IN_MINUTE, 1, "Pressing machine", &pressing_machine_worker, PRESSING_MACHINE);
    machines[PRESSING_MACHINE] = &pressing_machine;
    machine one_sided_sander(options.N_ONE_SIDED_SANDER, maintenance_time[1][0], 10 * SECONDS_IN_MINUTE, 10, "One sided sander", &one_sided_sander_worker, ONE_SIDED_SANDER);
    machines[ONE_SIDED_SANDER] = &one_sided_sander;
    machine aligner(options.N_ALIGNER, maintenance_time[2][0], 10 * SECONDS_IN_MINUTE, 10, "Aligner", &aligner_worker, ALIGNER);
    machines[ALIGNER] = &aligner;
    machine stretcher(options.N_STRETCHER, maintenance_time[3][0], 50 * SECONDS_IN_MINUTE, 2.5 * SECONDS_IN_MINUTE, "Stretcher", &stretcher_worker, STRETCHER);
    machines[STRETCHER] = &stretcher;
    machine double_sided_sander(options.N_DOUBLE_SIDED_SANDER, maintenance_time[4][0], 10 * SECONDS_IN_MINUTE, 10, "Double sided sander", &double_sided_sander_worker, DOUBLE_SIDED_SANDER);
    machines[DOUBLE_SIDED_SANDER] = &double_sided_sander;
    machine oiling_machine(options.N_OILING, maintenance_time[5][0], 0, 2, "Oiling machine", &oiling_machine_worker, OILING_MACHINE);
    machines[OILING_MACHINE] = &oiling_machine;

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, options.SIMULATION_DURATION); // time of simulation
    (new order_event)->Activate();
    (new supply_event)->Activate();
    (new maintenance_event)->Activate(Time + SECONDS_IN_HOUR * 8); // first maintenance after 8 hours not at the start of the simulation
    (new break_event)->Activate(Time + SECONDS_IN_HOUR * 2);       // first break after 2 hours not at the start of the simulation
    Run();

    print_stats(options);

    return ErrCode(SUCCESS);
}
