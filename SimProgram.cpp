#include "SimProgram.hpp"

using namespace std;

void help(const char *prog_name)
{
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
class material_warehouse : public Facility
{
private:
    unsigned warehouse_max_capacity;
    unsigned warehouse_current_capacity;

public:
    material_warehouse(const char *desc, unsigned max_capacity, unsigned current_capacity) : Facility()
    {
        this->warehouse_max_capacity = max_capacity;
        this->warehouse_current_capacity = current_capacity;
    }

    // send material every 24 hours and add it to the current warehouse capacity
    void add_material(unsigned amount)
    {
        // chceck if passed 24 hours since the last material was added
        if (Time < 86400 && Time > 86400 + 60) // TODO: NOT SURE ABOUT THIS THO
        {
            this->warehouse_current_capacity += amount; // increase the capacity of the warehouse by the amount of material
        }

        return;
    }

    int get_back(unsigned amount)
    {
        if (amount > this->warehouse_current_capacity) // chceck if the amount is not bigger than the capacity
        {
            return ErrCode(FAIL);
        }
        this->warehouse_current_capacity -= amount; // decrease the capacity of the warehouse by the amount of material

        double utilization = static_cast<double>(this->warehouse_current_capacity) / static_cast<double>(this->warehouse_max_capacity) * 100;

        // TODO:

        return ErrCode(SUCCESS);
    };
};

// ########## Machines for producing brake discs ##########
class machine : public Facility
{

public:
    enum machine_types type;

    // constructor
    machine(void){}; // TODO: MAYBE NOT NEEDED

    machine(const char *name, enum machine_types type) : Facility(name)
    {
        this->type = type;

        return;
    }
};

//----------------------------------------------- CONSTANTS, GLOBALS AND DATA STRUCTURES -----------------------------------------------

// ########## GLOBAL WAREHOUSE FOR MATERIAL ##########
material_warehouse warehouse("Material warehouse", 10000, 5000);

// //########## GLOBAL FACILITIES FOR PRODUCING ACTUAL BRAKE DISKS ##########
// Facility pressing_machine("Pressing machine");
// Facility one_sided_sander("Sander machine (one side only)");
// Facility aligner("Aligner machine");
// Facility stretcher("Stretching machine");
// Facility double_sided_sander("Sander machine (both sides)");
// Facility oiling_machine("Oiling machine");
// Facility packing("Packing brake disks to the boxes");
// Facility transport("Transporting brake disks to the another facility");

//----------------------------------------------- PROCESSES -----------------------------------------------

// ########## Simulation proccess for the order ##########
class Order : public Process
{
private:
    unsigned order_size;           // number of brake discs in the order
    unsigned order_id;             // id of the order
    material_warehouse *warehouse; // pointer to the material warehouse
    Queue *orders;
    double startTime;

public:
    // constructor
    Order() : Process()
    {
        cout << "Order constructor" << endl;
        this->order_size = static_cast<unsigned>(Uniform(1000, 20000)); // random number of brake discs in the order
    }

    void Behavior()
    {
        cout << "Order" << endl;
        Wait(60);               // wait one minute for the order to be processed
        this->startTime = Time; // save the time of order

        // create new batch
    }
};

// ########## Simulation proccess for the new batch of brake discs from the order ##########
class batch : public Process
{
private:
    unsigned batch_size;           // number of brake discs in the batch to be made
    material_warehouse *warehouse; // pointer to the material warehouse
    Queue *orders;                 // pointer to the queue of orders waiting to be processed

    double startTime; // time of the start of the batch

public:
    void Behavior()
    {
        // TODO:
    }
};

// ########## Simulation proccess for the maintenance of the machine ##########
class maintenance : public Process
{
private:
    machine *machine_maintained; // pointer to the machine that is being maintained

public:
    // constructor
    maintenance(machine *machine_maintained) : Process()
    {
        this->machine_maintained = machine_maintained;

        return;
    }

    void Behavior()
    {
        // TODO: THIS AS FIRST THING #################################
        Wait(Normal(maintenance_time[machine_maintained->type][0], maintenance_time[machine_maintained->type][1]));
        // Seize(*machine_maintained); // seize the machine for the maintenance
        //  Wait(Normal(maintenance_time[machine_maintained->type][0], maintenance_time[machine_maintained->type][1]));
        // Release(*machine_maintained); // release the machine after the maintenance

        return;
    }
};

//----------------------------------------------- EVENTS -----------------------------------------------

// ########## Event for the maintenance ##########
class maintenance_event : public Event
{
private:
    machine *machines; // pointer to the machines

public:
    void Behavior()
    {
        (new maintenance(machines))->Activate(); // activate the maintenance process
        Activate(Time + 8 * 60 * 60); // schedule the next maintenance event after 8 hours
    }
};

// ########## Generator for new orders ##########
class OrderGen : public Event
{

    // behaviour generates the new orders and activates the Order process
    void Behavior()
    {
        (new Order)->Activate();
        Activate(Time + Exponential(28800)); // order every 8 hours (exponential)
        cout << "OrderGen" << endl;
    }
};

int main(int argc, char *argv[])
{

    // parameter handling
    if (argc > 1)
    {
        help(argv[0]);
    }
    else
    {
        return ErrCode(FAIL);
    }

    random_device rand;       // a device that produces random numbers
    long seed_value = rand(); // get a random long number from the device rand
    RandomSeed(seed_value);

    Init(0, 100000); // time of simulation
    (new OrderGen)->Activate();
    Run();

    // print out statistics TODO: ...

    return ErrCode(SUCCESS);
}


// ended at Many fixes + machine_maintenance & machine classes added. Removed pre…