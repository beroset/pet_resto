#include "mbed.h"
#include "ROM.h"

// 0000 0000 1111 1111
PortInOut data(PortD, 0x00ff);  // PD7 to PD0

#if ROM6540
// we're using a 6540 which is 2K x 8
// 0001 1111 1111 1100
PortOut addr(PortE, 0x1ffc);  // PE12 to PE2

// 6502 clock
DigitalOut clock02(PB_8);

int ROM::get(int address) {
    clock02 = 0;
    int val = address << 2;
    addr.write(val);
    // pause if needed
    wait_us(1);
    clock02 = 1;
    // pause if needed
    wait_us(1);
    int mydata = data.read();
    clock02 = 0;
    return mydata;
}
#else
// we're using a 27C64A which is 8K x 8
// 0111 1111 1111 1100
PortOut addr(PortE, 0x7ffc);  // PE14 to PE2

DigitalOut ROM_G(PB_8);
DigitalOut ROM_EN(PB_9);
DigitalOut ROM_PROG(PB_15);

int ROM::get(int address) {
    int val = address << 2;
    addr.write(val);
    ROM_EN = 0;
    ROM_PROG = 1;
    ROM_G = 1;   // disable output
    wait_us(1);
    ROM_G = 0;
    wait_us(1);
    int mydata = data.read();
    ROM_G = 1;
    return mydata;
}

int ROM::put(int address, uint8_t byte) {
    int mydata = byte;
    ROM_EN = 1;  // disable ROM
    ROM_G = 1;   // disable output
    wait_us(2);  // 2us just for safety
    data.output();
    int val = address << 2;
    addr.write(val);
    data.write(byte);
    wait_us(2);  // 2us just for safety
    ROM_PROG = 1;
    ROM_EN = 0;
    wait_us(2);  // 2us chip enable to program low
    for (unsigned n=0; n < 25; ++n) {
        ROM_PROG = 0;
        wait_us(1000);  // 1m program pulse
        ROM_PROG = 1;
        wait_us(2);  // 2us program high to input
        data.input();
        ROM_G = 0;
        wait_us(1);
        mydata = data.read();
        ROM_G = 1;
        wait_us(2);
        if (mydata == byte) {  // verify was OK
            data.output();
            // perform n overprogram pulses
            for ( ;n; --n) {
                ROM_PROG = 0;
                wait_us(3000);  // 3ms overprogram pulse
                ROM_PROG = 1;
                wait_us(2);
            }
            data.input();
            wait_us(2);
            return 1;
        }
    }
    printf("failed to program 0x%x = %x (got %x instead)\n", address, byte, mydata);
    return 0;  // programming this byte failed!
}
#endif

void ROM::dump(Serial& pc, int start, int end) {
    const int linelen = 16;
    for (int myaddr = start; myaddr < end; myaddr += linelen) {
        pc.printf("%6.6x:", myaddr);
        for (int i=0; i < linelen; ++i) {
            int d = get(myaddr+i);
            pc.printf(" %2.2x", d);
        }
        pc.printf("\n");
    }
}

void ROM::copy(char *buffer, int start, int end) {
    for ( ; start != end; ++start) {
        *buffer = get(start);
        ++buffer;
    }
}

unsigned long ROM::program(char *buffer, int start, int end) {
    unsigned long count = 0;
    for ( ; start != end; ++start) {
        count += put(start, buffer[start]);
    }
    return count;
}
