#include "mbed.h"
#include "ROM.h"

/** Serial port for I/O
 *
 * 115200 baud, 8-bit data, no parity
 */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);

/** Green LED on board */
DigitalOut greenLED(LED1);
/** Blue LED on board */
DigitalOut blueLED(LED2);

/** RAM buffer holds ROM image */
char buffer[ROM::size()];

/** Returns true if character is a hex digit
 *
 * Intended as replacement for the standard function 
 * of the same name.
 *
 * \param ch Passed character to be checked
 * \return true if char is in 0-9a-fA-F
 */
bool isxdigit(char ch) {
    return 
        ( ch >= '0' && ch <= '9') 
        || (ch >= 'A' && ch <= 'F')
        || (ch >= 'a' && ch <= 'f');
}

/** Returns upper case version of alpha character
 *
 * The character is unchanged if it is not in a-z
 *
 * \param ch the character to be altered
 * \return the uppercase or unchanged character
 */
char toupper(char ch) {
    return (ch >= 'a' && ch <= 'z') ?
        ch -= ('a' - 'A') : ch;
}

/** Dumps buffer contents to serial port as ASCII dump
 *
 * Format is 6 character hex address, followed by ':' 
 * followed by memory contents as hex bytes separated 
 * by spaces, sixteen to a line.
 *
 * \param pc Serial port to which to dump
 * \param start Beginning address of dump
 * \param end One byte past last address of dump
 */
void dump(Serial& pc, unsigned start, unsigned end) {
    const unsigned linelen = 16;
    for (unsigned myaddr = start; myaddr < end; myaddr += linelen) {
        pc.printf("%6.6x:", myaddr);
        for (unsigned i=0; i < linelen; ++i) {
            unsigned d = buffer[myaddr+i];
            pc.printf(" %2.2x", d);
        }
        pc.printf("\n");
    }
}

/** Reads serial input into buffer
 *
 * The serial input is formatted in the same way as the
 * `dump` command produces, namely an address followed by
 * a colon ':' followed by a series of ASCII hex pairs
 * separated by spaces.  The read is terminated by a blank
 * line.
 *
 * \param pc Serial port
 * \param start Beginning index in buffer
 * \param end One beyond last index in buffer
 * \return number of bytes actually read
 */
unsigned long read(Serial& pc, unsigned start, unsigned end) {
    enum LineState { addr, bytehi, bytelo };
    LineState state = addr;
    unsigned address = 0;
    unsigned value = 0;
    unsigned long count = 0;
    bool initial = true;
    while (char ch = pc.getc()) {
        switch (state) {
            case addr:
                if (isxdigit(ch)) {
                    ch = toupper(ch);
                    ch -= '0';
                    if (ch > 9) {
                        ch -= 7;
                    }
                    address <<= 4;
                    address |= ch;
                    initial = false;
                } else if (ch == ' ') {
                    // ignore spaces
                } else if (ch == ':') {
                    state = bytehi;
                } else if (ch == '\n') {
                    if (initial) {
                        // just ignore if it's before first address
                    } else {
                        // end of file
                        return count;
                    }
                } else {
                    pc.printf("Illegal character \"%c\" in input\n", ch);
                    return count;
                }
                break;
            case bytehi:
                if (isxdigit(ch)) {
                    ch = toupper(ch);
                    ch -= '0';
                    if (ch > 9) {
                        ch -= 7;
                    }
                    value = (ch <<= 4);
                    state = bytelo;
                } else if (ch == ' ') {
                    // ignore space chars
                } else if (ch == '\n') {
                    address = 0;
                    state = addr;
                }
                break;
            case bytelo:
                if (isxdigit(ch)) {
                    ch = toupper(ch);
                    ch -= '0';
                    if (ch > 9) {
                        ch -= 7;
                    }
                    value |= ch;
                    if (address < end) {
                        buffer[address] = value;
                        ++address;
                        ++count;
                        state = bytehi; 
                    } else {
                        pc.printf("Attempted to write beyond end of buffer\n");
                        return count;
                    }
                } else if (ch == ' ') {
                    // ignore space chars
                } else if (ch == '\n') {
                    // unexpected end of file
                    pc.printf("Unexpected end of file mid data byte\n");
                    return count;
                }
                break;
        }
    }
    pc.printf("unknown error\n");
    return count;
}

/** Calculate the CRC of passed buffer and size
 *
 * This function duplicates the function of the Linux `ckmem`
 * utility which calculates the CRC of a file and its length. 
 * The length, for the purposes of CRC calculation, is encoded
 * as a little-endian number with the fewest number of bytes
 * possible to represent the number.  So for example, a buffer
 * that is 0x1234 bytes long would be appended with 0x34, 0x12
 * for the purposes of CRC calculation.  
 *
 * \param buffer Pointer to the buffer
 * \param size Size of buffer
 * \return The final CRC of the buffer+size
 */
unsigned long crc(void *buffer, size_t size) 
{
    MbedCRC<POLY_32BIT_ANSI, 32> ct(0,0xffffffff, false, false);
    uint32_t crc = 0;
    ct.compute_partial_start(&crc);
    ct.compute_partial(buffer, size, &crc);
    while (size) {
        ct.compute_partial((void *)&size, 1, &crc);
        size >>= 8;
    }
    ct.compute_partial_stop(&crc);
    return crc;
}

/** Fetches a line from the passed Serial port no longer than `len`
 *
 * The returned string will be NUL-termminated.  Note that the 
 * passed length does NOT include the terminating NUL character.
 *
 * \param buffer The buffer into which we put the string.
 * \param len Size of buffer-1 to allow for NUL termination
 * \param pc Serial port
 * \return Number of characters read
 */
std::size_t getline(char *buffer, std::size_t len, Serial& pc) {
    std::size_t i = 0;
    for ( ; i < len; ++i) {
        char ch = pc.getc();
        if (ch == '\n') {
            break;
        } else {
            *buffer++ = ch;
        }
    }
    *buffer = '\0';
    return i;
}

/** Prints the version of this tool 
 */
void version() {
    printf("ROMtool v. 1.00\n");
}

int main()
{
    version();
    ROM rom;
    char cmdbuffer[20];
    while (1) {
        printf(">");
        getline(cmdbuffer, 19, pc);
        char ch = toupper(cmdbuffer[0]);
        switch(ch) {
            case 'D':
                printf("Dump\n");
                greenLED = 1;
                dump(pc, 0, rom.size());
                greenLED = 0;
                break;
            case 'R':
                printf("Read\n");
                greenLED = 1;
                rom.copy(buffer);
                greenLED = 0;
                break;
            case 'V':
                printf("Version\n");
                version();
                break;
            case 'P':
                printf("Program\n");
                blueLED = 1;
                {
                    auto result = rom.program(buffer, 0, rom.size());
                    printf("Programmed 0x%lx (dec %ld) bytes into device\n", result, result);
                }
                blueLED = 0;
                break;
            case 'F':
                printf("File\n");
                greenLED = 1;
                {
                    auto result = read(pc, 0, rom.size());
                    printf("Loaded 0x%lx (dec %ld) bytes into buffer\n", result, result);
                }
                greenLED = 0;
                break;
            case 'C':
                printf("CRC\n");
                {
                    auto memcrc = crc(buffer, rom.size());
                    printf("CRC is : 0x%lx (dec %lu)\n", memcrc, memcrc);
                }
                break;
            case '\n':
            case '\r':
                break;
            default:
                pc.printf("unknown command %c\n", ch);
        }
    }
}
