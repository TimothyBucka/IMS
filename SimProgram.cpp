#include "SimProgram.hpp"

using namespace std;

void help(const char *prog_name) {
    cout << prog_name << " program implements the SHO of the engineering company." << endl;
    cout << "Subject: IMS" << endl;
    cout << "Authors:  Timotej Bucka (xbucka00) " << endl;
    cout << "          Adam Pap (xpapad11) " << endl;
    cout << "Contacts: xbucka00@stud.fit.vutbr.cz " << endl;
    cout << "          xpapad11@stud.fit.vutbr.cz " << endl;
}

//----------------------------------------------- QUEUES -----------------------------------------------

// ########## Queue for orders ##########
Queue orders("Brake discs order:");

//----------------------------------------------- FACILITIES -----------------------------------------------

// ########## Material warehouse ##########
class material_warehouse : public Facility {
private:
    float max_capacity;
    float current_capacity;

public:
    float get_current_capacity() { return this->current_capacity; }

    material_warehouse(const char *desc, float max_capacity, float current_capacity) : Facility() {
        this->max_capacity = max_capacity;
        this->current_capacity = current_capacity;
    }

    // send material every 24 hours and add it to the current warehouse capacity
    bool add_material(unsigned amount) {
        if (this->current_capacity + amount > this->max_capacity)
        {
            cout << "WAREHOUSE: warehouse full - " << this->current_capacity + amount - this->max_capacity << " kg not added" << endl; // TODO: ...
            this->current_capacity = this->max_capacity;
            return false;
        }

        this->current_capacity += amount; // increase the capacity of the warehouse by the amount of material

        return true;
    }

    bool use_material(float amount) {
        if (amount > this->current_capacity) // chceck if the amount is not bigger than the capacity
        {
            cout << "WAREHOUSE: amount of material is bigger than the capacity" << endl; // TODO: ...
            return false;
        }
        this->current_capacity -= amount; // decrease the capacity of the warehouse by the amount of material

        float utilization = static_cast<float>(this->current_capacity) / static_cast<float>(this->max_capacity) * 100;

        // TODO:

        return true;
    };
};

// ########## Machines for producing brake discs ##########
class machine : public Facility {

public:
    enum machine_types type;

    // constructor
    machine(void){}; // TODO: MAYBE NOT NEEDED

    machine(const char *name, enum machine_types type) : Facility(name) {
        this->type = type;

        return;
    }
};

//----------------------------------------------- CONSTANTS, GLOBALS AND DATA STRUCTURES -----------------------------------------------

// ########## GLOBAL WAREHOUSE FOR MATERIAL ##########
material_warehouse warehouse("Material warehouse", MATERIAL_WAREHOUSE_CAPACITY, INITIAL_MATERIAL_WAREHOUSE_WEIGHT);
unsigned order_id_count = 0;

// //########## GLOBAL FACILITIES FOR PRODUCING ACTUAL BRAKE DISKS ##########
// Facility pressing_machine("Pressing machine");
// Facility one_sided_sander("Sander machine (one side only)");
// Facility aligner("Aligner machine");
// Facility stretcher("Stretching machine");
// Facility float_sided_sander("Sander machine (both sides)");
// Facility oiling_machine("Oiling machine");
// Facility packing("Packing brake disks to the boxes");
// Facility transport("Transporting brake disks to the another facility");

//----------------------------------------------- PROCESSES -----------------------------------------------

// ########## Simulation proccess for the order ##########
class Order : public Process {
private:
    unsigned order_size;           // number of brake discs in the order
    float amount_of_material;      // amount of material needed for the order
    unsigned order_id;             // id of the order
    Queue *orders;
    float startTime;

public:
    // constructor
    Order() : Process() {
        this->order_size = static_cast<unsigned>(Uniform(1000, 20000));      // random number of brake discs in the order
        this->amount_of_material = PIECE_MATERIAL_WEIGHT * this->order_size; // amount of material needed for the order
    }

    void Behavior() {
        cout << "Order:" << endl;
        cout << "\tOrder came in: " << Time / SECONDS_IN_HOUR << endl;
        cout << "\tOrder size: " << this->order_size << endl;
        cout << "\tAmount of material: " << this->amount_of_material << endl;
        // cout << "Warehouse cap: " << warehouse->get_current_capacity() << endl;

        Wait(SECONDS_IN_MINUTE);

        this->startTime = Time;                          // save the time of order

        this->order_id = order_id_count++;

        if(warehouse.use_material(this->amount_of_material)) {

        } else {
            // TODO: not enough material
            cout << "ORDER N." << this->order_id <<": not enough material" << endl;
            return;
        }
        // create new batch
    }
};

// ########## Simulation proccess for the new batch of brake discs from the order ##########
class batch : public Process {
private:
    unsigned batch_size;           // number of brake discs in the batch to be made
    material_warehouse *warehouse; // pointer to the material warehouse
    Queue *orders;                 // pointer to the queue of orders waiting to be processed

    float startTime; // time of the start of the batch

public:
    void Behavior() {
        // TODO:
    }
};

// ########## Simulation proccess for the maintenance of the machine ##########
class maintenance : public Process {
private:
    machine *machine_maintained; // pointer to the machine that is being maintained

public:
    // constructor
    maintenance(machine *machine_maintained) : Process() {
        this->machine_maintained = machine_maintained;

        return;
    }

    void Behavior() {
        // TODO: THIS AS FIRST THING #################################
        Wait(Normal(maintenance_time[machine_maintained->type][0], maintenance_time[machine_maintained->type][1]));
        // Seize(*machine_maintained); // seize the machine for the maintenance
        //  Wait(Normal(maintenance_time[machine_maintained->type][0], maintenance_time[machine_maintained->type][1]));
        // Release(*machine_maintained); // release the machine after the maintenance

        return;
    }
};

class Supply : public Process {
public:
    Supply() : Process() {}

    void Behavior() {
        cout << "Supply of 5000: "<<endl;

        if (warehouse.add_material(MATERIAL_SUPPLY_WEIGHT)) {

        } else {
            return;
        }
    }
};


//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
class maintenance_event : public Event {
private:
    machine *machines; // pointer to the machines

public:
    void Behavior() {
        (new maintenance(machines))->Activate(); // activate the maintenance process
        Activate(Time + 8 * 60 * 60);            // schedule the next maintenance event after 8 hours
    }
};

// ########## Generator for new orders ##########
class OrderGen : public Event {
    // behaviour generates the new orders and activates the Order process
    void Behavior() {
        (new Order)->Activate();
        Activate(Time + Exponential(SECONDS_IN_HOUR * 8)); // order every 8 hours (exponential)
    }
};

// ########## Generator for supplies ##########
class SupplyGen : public Event {
    void Behavior() {
        (new Supply)->Activate();
        Activate(Time + Normal(SECONDS_IN_DAY, SECONDS_IN_HOUR * 2)); // supply every 24 hours (exponential
    }
};

int main(int argc, char *argv[]) {

    // parameter handling
    if (argc > 1) {
        help(argv[0]);
    }

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, SECONDS_IN_DAY * 1); // time of simulation
    (new OrderGen)->Activate();
    (new SupplyGen)->Activate();
    Run();

    // print out statistics TODO: ...

    cout << "Warehouse cap: " << warehouse.get_current_capacity() << endl;

    return ErrCode(SUCCESS);
}

// ended at Many fixes + machine_maintenance & machine classes added. Removed pre…