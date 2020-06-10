#ifndef BIOS_HLE_HPP
#define BIOS_HLE_HPP
#include <vector>
#include "emotion.hpp"

enum CALL_PARAMS
{
    RETURN = 2,
    PARAM0 = 4,
    PARAM1 = 5,
    PARAM2 = 6,
    PARAM3 = 7,
    PARAM4 = 8,
    PARAM5 = 9
};

struct thread_hle
{
    uint32_t stack_base;
    uint32_t heap_base;
    uint8_t priority;
};

struct INTC_handler
{
    uint32_t cause;
    uint32_t addr;
    uint32_t next;
    uint32_t arg;
};

namespace GS
{
    class GraphicsSynthesizer;
}

class BIOS_HLE
{
    private:
        Emulator* e;
        GS::GraphicsSynthesizer* gs;
        std::vector<thread_hle> threads;
        std::vector<INTC_handler> intc_handlers;

        void sw(uint32_t& address, uint32_t value);
        void store_INTC_handler(INTC_handler& h);
        void assemble_interrupt_handler();
    public:
        BIOS_HLE(Emulator* e, GS::GraphicsSynthesizer* gs);

        void reset();

        void hle_syscall(EmotionEngine& cpu, int op);

        void reset_EE(EmotionEngine& cpu);
        void set_GS_CRT(EmotionEngine& cpu);
        void set_VBLANK_handler(EmotionEngine& cpu);
        void add_INTC_handler(EmotionEngine& cpu);
        void enable_INTC(EmotionEngine& cpu);
        void init_main_thread(EmotionEngine& cpu);
        void init_heap(EmotionEngine& cpu);
        void get_heap_end(EmotionEngine& cpu);
        void set_GS_IMR(EmotionEngine& cpu);
        void get_memory_size(EmotionEngine& cpu);
};

#endif // BIOS_HLE_HPP
