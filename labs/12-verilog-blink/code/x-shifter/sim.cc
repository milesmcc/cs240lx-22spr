#include <stdlib.h>
#include "Vshifter.h"
#include "verilated.h"

void check(Vshifter* dut, uint32_t in) {
    dut->in = in;
    dut->eval();
    printf("check: %d << 1 = %d (expect: %d)\n", in, dut->shifted, in << 1);
    assert(dut->shifted == in << 1);
}

int main() {
    Vshifter* dut = new Vshifter;

    for (int i = 0; i < 10; i++) {
        uint32_t a = rand();
        check(dut, a);
    }

    return 0;
}

