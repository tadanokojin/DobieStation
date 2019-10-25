#include "vu_jit.hpp"
#include "vu_jit64.hpp"
#include "vu.hpp"

#include "vu_jittrans.hpp"
#include "core/jitcommon/jitcache.hpp"
#include "core/jitcommon/emitter64.hpp"

#include "core/errors.hpp"

namespace VU_JIT
{

VU_JIT64 jit64;

uint16_t run(VectorUnit *vu)
{
    return jit64.run(*vu);
}

void reset()
{
    jit64.reset();
}

void set_current_program(uint32_t crc)
{
    jit64.set_current_program(crc);
}

};
