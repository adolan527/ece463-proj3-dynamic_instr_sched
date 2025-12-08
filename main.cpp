#include <iostream>

#include "Simulator.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        printf("Usage: sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>");
    }
    auto rob_size = atoi(argv[1]);
    auto iq_size = atoi(argv[2]);
    auto width = atoi(argv[3]);
    char *tracefile = argv[4];

    Simulator simulator(rob_size,iq_size,width,tracefile);
    simulator.Run();

    return 0;
}
