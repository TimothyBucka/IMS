#include "SimProgram.hpp"

using namespace std;

class Order : public Process {
public:
    Order() {
        cout << "Order constructor" << endl;
    }

    void Behavior() {
        cout << "Order" << endl;
    }
};

class OrderGen : public Process {
    void Behavior() {
        (new Order)->Activate();
        Activate(Time + Exponential(28800));
        cout << "OrderGen" << endl;
    }
};

int main(int argc, char *argv[]) {
    Init(0, 28800);
    (new OrderGen)->Activate();
    return ErrCode(SUCCESS);
}