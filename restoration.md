% PET restoration
% E. Beroset
% 2 January 2020

# Introduction
I've owned my Commodore PET since 1978.  For about the last 25 years, the PET hasn't worked due to one or more problems. In 2019, I diagnosed and fixed a problem with the power supply (faulty filter capacitor), but there are still other issues.  This document is a record of my attempts to fix up the old PET.

## Symptom
The current symptom is that the screen is all scrambled.  That is, instead of reporting "7167 BYTES FREE" as a healthy PET would, I get a scrambled screen as shown below in Figure 1. 

![scrambled screen](screen_shot.jpg)

To rectify this problem, I first tried moving RAM chips around.  I have the 18 6550 RAM chips (16 for main memory and 2 for video memory) labelled as A through S (omitting "O" because it looks too much like a zero).  It turns out that only 6 chips need to be installed in the machine for it to boot.  The two video RAM chips and only 4 of the memory chips in locations I1, J1, I2 and J2.  Initially,  the chips were I1 = A, J1 = B, I2 = C, J2 = D, C3 = E, C4 = F.  Changing to S,G,J,K,L,M produced a nearly identical display, suggesting that the problem lies elsewhere.  I am starting to suspect ROMs.

## ROM diagnostics
I recall that when the PET boots normally, it turns off the cassette motor within about 1 second.  That is still happening, so I am starting to wonder about perhaps the character ROM or other ROMs being bad.  Figuring out which one will need a bit of sleuthing.  I first tried to swap the character ROM (010) with H1 (011).  As expected, the initial random screen no longer had any identifiable characters, suggesting that the character ROM is not completely bad.  I next replaced char ROM (010) and removed H1 (011) and H2 (013) and the result was essentially the same as when they are present.  While not definitive, this suggests that the problem may be in the higher ROMs.  I next removed H6 and H5 and the problem was nearly identical.  When I also removed H3, the cassette motor no longer turns off but the display is nearly identical.  Finally, I removed all ROMs and the display is about the same, but the cassette motor stays on.  From left to right looking from the front of the PET, the ROMs are as shown in the table below.  Note that the range of E800 to EFFF is "missing" because that range is mapped to IO.  Specifically, PIA 1 is E810 to E81F, PIA 2 is E820 to E82F, the VIA is E840 to E84F and on models equipped with it (this one is not) the CRTC is mapped to E880 to E88F.

| Pos | ROM | Start | End  |
|-----|-----|-------|------|
| H7 | 018 | F800 | FFFF |
| H6 | 014 | D800 | DFFF |
| H5 | 012 | C800 | CFFF |
| H4 | 016 | F000 | F7FF |
| H3 | 015 | E000 | E7FF |
| H2 | 013 | D000 | D7FF |
| H1 | 011 | C000 | C7FF |

The 6540 ROM pinout is shown in Figure 2.  To verify the ROMs, I will use an Arduino-compatible development board I have available (the Nucleo F207ZG from ST Microelectronics) and read the contents of the ROM and stream to a serial port.  The sequence for addressing the ROM is as follows:

 1. Set 02 low
 2. Set Chip Select (CS1, CS2 to +5V, CS3, CS4, CS5 to GND)
 3. Set Address lines (A0 through A10)
 4. Allow at least 80ns to settle address lines
 5. Set 02 high
 6. Keep high for min 350ns (addr may change during this time)
 7. Read D0-D7
 8. Set 02 low

 See the timing diagram in Figure 3. 

![6540 ROM pinout](6540.png)

![6540 timing diagram](6540_timing.png)


### ROM tester
I'm using my Nucleo F207ZG board to do ROM testing.  Due to the fact that not all pins are easy to get to, I'm going to use PortE 2-12 for the address lines A0-A10 and PortD 0-7 for the D0-D7 lines.  All chip selects will be hardwired and the 02 (clock) line will be PB8.  Since it's a 300ns chip, I might need to add some delay.  I will use a 1us delay, which should be more than enough to allow me to read the contents of the ROM.  The ROM will be powered from +5VDC, but will be controlled via I/O at 3.3V, since that's what's used for the development board I'm using.  This works because the old TTL stuff only needed +2.0V or greater for `1`.  All of the ports on my development board are 5V tolerant as long as the pull-up and pull-down resistors are disabled.


|  Nucleo board |||  6540 ROM  |
|----|-----|------|------|-----|
| CN | Pin | Desc | Desc | pin |
|----|-----|------|------|-----|
|  9 |  14 | PE2  |  A0  |  5  |
|  9 |  22 | PE3  |  A1  |  6  |
|  9 |  16 | PE4  |  A2  |  7  |
|  9 |  18 | PE5  |  A3  |  8  |
|  9 |  20 | PE6  |  A4  |  9  |
| 10 |  20 | PE7  |  A5  | 10  |
| 10 |  18 | PE8  |  A6  | 15  |
| 10 |   4 | PE9  |  A7  | 14  |
| 10 |  24 | PE10 |  A8  | 13  |
| 10 |   6 | PE11 |  A9  | 11  |
| 10 |  26 | PE12 |  A10 | 18  |
|  9 |  25 | PD0  |  D0  | 26  |
|  9 |  27 | PD1  |  D1  | 25  |
|  8 |  12 | PD2  |  D2  | 24  |
|  9 |  10 | PD3  |  D3  | 23  |
|  9 |   8 | PD4  |  D4  | 22  |
|  9 |   6 | PD5  |  D5  | 21  |
|  9 |   4 | PD6  |  D6  | 20  |
|  9 |   2 | PD7  |  D7  | 19  |
|  7 |   2 | PB8  |  CLK | 16  |
|  8 |   9 | +5V  |  Vdd | 12  |
|  8 |   9 | +5V  |  CS1 | 17  |
|  8 |   9 | +5V  |  CS2 | 27  |
|  8 |  13 | GND  |  Vss |  1  |
|  8 |  13 | GND  | #CS3 |  4  |
|  8 |  13 | GND  | #CS4 |  3  |
|  8 |  13 | GND  | #CS5 |  2  |

### ROM reading software for Nucleo board

```
#include "mbed.h"

//------------------------------------
// 115200 baud, 8-bit data, no parity
//------------------------------------

Serial pc(SERIAL_TX, SERIAL_RX, 115200);

DigitalOut myled(LED1);
DigitalOut blue(LED2);
DigitalIn button(BUTTON1);
DigitalOut clock02(PB_8);

// 0000 0000 1111 1111
PortIn  data(PortD, 0x00ff);  // PD7 to PD0

// 0001 1111 1111 1100
PortOut addr(PortE, 0x1ffc);  // PE12 to PE2

int getROM(int address) {
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

int main()
{
    const int linelen = 16;
    while (1) {
        if (button) {
            myled = 1;
            for (int myaddr = 0; myaddr < 2048; myaddr += linelen) {
                pc.printf("%4.4x:", myaddr);
                for (int i=0; i < linelen; ++i) {
                    int d = getROM(myaddr+i);
                    pc.printf(" %2.2x", d);
                }
                pc.printf("\n");
            }
            myled = 0;
        }
    }
}
```

The actual rig is shown in Figure 4 with one of the ROMs under test.

![ROM test rig](rom_test.jpg)

### ROM testing results
The ROM test showed problems with two ROMs.  The 012 ROM (C800-CFFF) and the 010 ROM (character generator).  The symptoms were slightly different for each.  For the 012 ROM, most of the contents were intact but some of the bytes were 0xff.  The bytes were always odd addresses starting at 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f and 0x51 and then every 0x80 bytes after those.  For the 010 ROM, the problem was that the high half of the ROM was a copy of the low half as though the A10 address line were stuck low.  While this test discovered problems with 2 ROMs, there are a number of aspects not tested, including whether all of the CS lines actually work and whether the ROM is too slow (the Nucleo program inserts a 1us pause, while the real timing spec is 350ns).  Still, this was fruitful and I may devise a means by which a more common part could be substituted.

## RAM testing
The 6550 SRAM chips are no longer made, but 2114 chips are still available.  They're not quite pin compatible but it's possible to make an adapter and I might actually have some 2114 chips somewhere.  The pinout is shown in Figure 5.

![MOS 6550 Pinout](6550.jpg)

The table below is the circuit I'll use to test the RAM chips.  The built circuit is shown below as Figure 6.

|  Nucleo board |||  6550 RAM  |
|----|-----|------|------|-----|
| CN | Pin | Desc | Desc | pin |
|----|-----|------|------|-----|
|  9 |  14 | PE2  |  A0  |  1  |
|  9 |  22 | PE3  |  A1  |  2  |
|  9 |  16 | PE4  |  A2  |  3  |
|  9 |  18 | PE5  |  A3  |  4  |
|  9 |  20 | PE6  |  A4  |  5  |
| 10 |  20 | PE7  |  A5  |  6  |
|  7 |   2 | PB8  |  CLK |  7  |
| 10 |  18 | PE8  |  A6  |  8  |
| 10 |   4 | PE9  |  A7  |  9  |
| 10 |  24 | PE10 |  A8  | 10  |
| 10 |   6 | PE11 |  A9  | 11  |
|  7 |   4 | PB9  |  R/W | 12  |
|  9 |  25 | PD0  |  DB0 | 13  |
|  9 |  27 | PD1  |  DB1 | 14  |
|  8 |  12 | PD2  |  DB2 | 15  |
|  9 |  10 | PD3  |  DB3 | 16  |
|  8 |   9 | +5V  |  Vdd | 17  |
|  8 |  13 | GND  | #CS4 | 18  |
|  8 |  13 | GND  | #CS3 | 19  |
|  8 |   9 | +5V  |  CS2 | 20  |
|  8 |   9 | +5V  |  CS1 | 21  |
|  8 |  13 | GND  |  Vss | 22  |

![MOS 6550 test circuit](ram_test.jpg)

### Test algorithm
There are a number of tests that could be used, but we can start with the obvious and quick ones and refine those later if needed.  As with the ROM testing, I'm not planning on testing the CS lines or the timing, but only the most basic function of a static RAM -- does it remember what it's asked to?  To that end, I'll use a simple test regimen using the following patterns:

 1. all zeroes
 2. all ones
 3. 0101 pattern
 4. 1010 pattern

### RAM test program

```
#include "mbed.h"

//------------------------------------
// 115200 baud, 8-bit data, no parity
//------------------------------------

Serial pc(SERIAL_TX, SERIAL_RX, 115200);

DigitalOut myled(LED1);
DigitalOut blue(LED2);
DigitalOut red(LED3);
DigitalIn button(BUTTON1);
DigitalOut clock02(PB_8);
DigitalOut reader(PB_9);

// 0000 0000 0000 1111
PortInOut  data(PortD, 0x000f);  // PD3 to PD0

// 0000 1111 1111 1100
PortOut addr(PortE, 0x0ffc);  // PE11 to PE2

int getRAM(int address) {
    clock02 = 0;
    reader = 1;
    data.input();
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

void putRAM(int address, int nybble) {
    clock02 = 0;
    reader = 0;
    data.output();
    int val = address << 2;
    addr.write(val);
    data.write(nybble);
    // pause if needed
    wait_us(1);
    clock02 = 1;
    // pause if needed
    wait_us(1);
    clock02 = 0;
    data.input();
    reader = 1;
}

void fillRAM(int nybble) {
    for (int myaddr = 0; myaddr < 1024; ++myaddr) { 
        putRAM(myaddr, nybble);
    }
}

int verifyRAM(int nybble) {
    int errors = 0;
    for (int myaddr = 0; myaddr < 1024; ++myaddr) { 
        int got = getRAM(myaddr);
        if (got != nybble) {
            if (!errors) {
                pc.printf("Error at %4.4x: wanted %2.2x got %2.2x\n", myaddr, nybble, got);
            }
            ++errors;
        }
    }
    return errors;
}

int checkRAM(int value)
{
    int ok = 1;
    myled = 1;
    fillRAM(value);
    myled = 0;
    int errors = verifyRAM(value);
    if (!errors) {
        pc.printf("Verified %2.2x\n", value);
    } else {
        pc.printf("ERRORS (%d) with %2.2x\n", errors, value);
        ok = 0;
    }
    return ok;
}

int main()
{
    data.mode(PullNone);
    while (1) {
        if (button) {
            blue = 0;
            red = 0;
            int ok = checkRAM(0x0);
            ok &= checkRAM(0xf);
            ok &= checkRAM(0x5);
            ok &= checkRAM(0xA);
            if (ok) {
                blue = 1;
                red = 0;
            } else {
                blue = 0;
                red = 1;
            }
        }
    }
}
```

### RAM test results
As described above, I labeled the 6550 chips A through S (omitting the letter 'O' because it looks too much like a zero) and tested.  Only the failing chips are detailed here:

#### B
```
Verified 00
Error at 0000: wanted 0f got 0b
ERRORS (1024) with 0f
Error at 0000: wanted 05 got 01
ERRORS (1024) with 05
Verified 0a
```

#### I

```
Error at 0001: wanted 00 got 0b
ERRORS (911) with 00
Error at 0001: wanted 0f got 0b
ERRORS (955) with 0f
Error at 0001: wanted 05 got 0b
ERRORS (966) with 05
Error at 0001: wanted 0a got 09
ERRORS (850) with 0a
```

#### S
```
Error at 0000: wanted 00 got 07
ERRORS (1024) with 00
Error at 0000: wanted 0f got 07
ERRORS (1024) with 0f
Error at 0000: wanted 05 got 07
ERRORS (1024) with 05
Error at 0000: wanted 0a got 07
ERRORS (1024) with 0a
```

Since these are 1K x 4 bit RAM, B and S clearly have some stuck bits.  In particular, B seems to have DB2 stuck at 0 for all addresses, and S has all memory stuck at 0x7.  Chip I apparently has something of a soft error in that it doesn't show all 1024 bytes bad, but the vast majority of them with 0xb or 0x9 as the content.  It might be interesting at some point to decap these and see if it's possible to pinpoint (and maybe even fix?) the precise error on the die.

## Preliminary Results
I have ordered 4 new 6550 chips (new old stock) from a guy on Ebay and have also ordered a board that sits between the 6502 and the socket and effectively replaces the stock ROM and RAM with modern-ish replacements.  I think I will probably wait until I receive that to complete the restoration.


