#ifndef ROM_H
#define ROM_H
#define ROM6540 0
class ROM {
public:
    int get(int address);
    int put(int address, uint8_t byte);
    void dump(Serial& pc, int start = 0, int end = size());
    void copy(char *buffer, int start = 0, int end = size());
    unsigned long program(char *buffer, int start, int end);
#if ROM6540 
    static constexpr int size() { return 2 * 1024; }
#else
    static constexpr int size() { return 8 * 1024; }
#endif
};
#endif // ROM_H
