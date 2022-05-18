#include <stdlib.h>
#include "Vsubtractor.h"
#include "verilated.h"

void check(Vsubtractor* dut, uint32_t a, uint32_t b) {
    dut->a = a;
    dut->b = b;
    dut->eval();
    printf("check: %d - %d = %d\n", a, b, dut->diff);
    assert(dut->diff == a - b);
}

int main() {
    Vsubtractor* dut = new Vsubtractor;

    for (int i = 0; i < 10; i++) {
        uint32_t a = rand();
        uint32_t b = rand();
        check(dut, a, b);
    }

    check(dut, 0, 0);
    check(dut, -1, 0);
    check(dut, 0, -1);
    check(dut, 1, -1);
    check(dut, -1, 1);

    return 0;
}

