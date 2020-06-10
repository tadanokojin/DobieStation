#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "ee/intc.hpp"
#include "gs.hpp"
#include "errors.hpp"

/**
This is a manager for the gsthread. It deals with communication
in/out of the GS, mostly with the GIF but also with the privileged
registers.
**/

namespace GS
{
    GraphicsSynthesizer::GraphicsSynthesizer(INTC* intc)
        : intc(intc), frame_complete(false),
        output_buffer1(nullptr), output_buffer2(nullptr)
    {
    }

    GraphicsSynthesizer::~GraphicsSynthesizer()
    {
        gs_thread.exit();

        delete[] output_buffer1;
        delete[] output_buffer2;
    }

    void GraphicsSynthesizer::reset()
    {
        gs_thread.reset();

        if (!output_buffer1)
            output_buffer1 = new uint32_t[1920 * 1280];

        if (!output_buffer2)
            output_buffer2 = new uint32_t[1920 * 1280];

        current_lock = std::unique_lock<std::mutex>();
        using_first_buffer = true;
        frame_count = 0;
        set_CRT(false, 0x2, false);
        reg.reset(false);
    }

    void GraphicsSynthesizer::start_frame()
    {
        frame_complete = false;
    }

    bool GraphicsSynthesizer::is_frame_complete() const
    {
        return frame_complete;
    }

    void GraphicsSynthesizer::set_CRT(bool interlaced, int mode, bool frame_mode)
    {
        reg.set_CRT(interlaced, mode, frame_mode);

        fifo::payload payload;
        payload.crt_payload = { interlaced, mode, frame_mode };

        gs_thread.send_message({ fifo::command::set_crt_t, payload });
    }

    uint32_t* GraphicsSynthesizer::get_framebuffer()
    {
        uint32_t* out;
        fifo::return_message data;

        gs_thread.wait_for_return(fifo::return_command::render_complete_t, data);

        if (using_first_buffer)
        {
            while (!output_buffer1_mutex.try_lock())
            {
                printf("[GS] buffer 1 lock failed!\n");
                std::this_thread::yield();
            }
            current_lock = std::unique_lock<std::mutex>(output_buffer1_mutex, std::adopt_lock);
            out = output_buffer1;
        }
        else
        {
            while (!output_buffer2_mutex.try_lock())
            {
                printf("[GS] buffer 2 lock failed!\n");
                std::this_thread::yield();
            }
            current_lock = std::unique_lock<std::mutex>(output_buffer2_mutex, std::adopt_lock);
            out = output_buffer2;
        }
        using_first_buffer = !using_first_buffer;
        return out;
    }

    void GraphicsSynthesizer::set_CSR_FIFO(uint8_t value)
    {
        reg.CSR.FIFO_status = value;
    }

    void GraphicsSynthesizer::set_VBLANK(bool is_VBLANK)
    {
        fifo::payload payload;
        payload.vblank_payload = { is_VBLANK };

        gs_thread.send_message({ fifo::command::set_vblank_t, payload });
        gs_thread.wake_thread();
        reg.set_VBLANK(is_VBLANK);

        if (is_VBLANK)
        {
            printf("[GS] VBLANK start\n");
            intc->assert_IRQ((int)Interrupt::VBLANK_START);
        }
        else
        {
            printf("[GS] VBLANK end\n");
            intc->assert_IRQ((int)Interrupt::VBLANK_END);
            frame_count++;
        }
    }

    void GraphicsSynthesizer::assert_VSYNC()
    {
        fifo::payload payload;
        payload.no_payload = {};

        gs_thread.send_message({ fifo::command::assert_vsync_t, payload });

        if (reg.assert_VSYNC())
            intc->assert_IRQ((int)Interrupt::GS);
    }

    void GraphicsSynthesizer::assert_FINISH()
    {
        fifo::payload payload;
        payload.no_payload = { };

        gs_thread.send_message({ fifo::command::assert_finish_t, payload });

        if (reg.assert_FINISH())
            intc->assert_IRQ((int)Interrupt::GS);
    }

    void GraphicsSynthesizer::render_CRT()
    {
        fifo::payload payload;

        if (using_first_buffer)
            payload.render_payload = { output_buffer1, &output_buffer1_mutex };
        else
            payload.render_payload = { output_buffer2, &output_buffer2_mutex }; ;

        gs_thread.send_message({ fifo::command::render_crt_t, payload });
        gs_thread.wake_thread();
    }

    uint32_t* GraphicsSynthesizer::render_partial_frame(uint16_t& width, uint16_t& height)
    {
        fifo::payload payload;

        if (using_first_buffer)
            payload.render_payload = { output_buffer1, &output_buffer1_mutex };
        else
            payload.render_payload = { output_buffer2, &output_buffer2_mutex }; ;

        gs_thread.send_message({ fifo::command::memdump_t,payload });
        gs_thread.wake_thread();
        fifo::return_message data;
        gs_thread.wait_for_return(fifo::return_command::gsdump_render_partial_done_t, data);

        width = data.payload.xy_payload.x;
        height = data.payload.xy_payload.y;

        if (using_first_buffer)
        {
            using_first_buffer = false;
            current_lock = std::unique_lock<std::mutex>(output_buffer1_mutex);
            return output_buffer1;
        }
        else
        {
            using_first_buffer = true;
            current_lock = std::unique_lock<std::mutex>(output_buffer2_mutex);
            return output_buffer2;
        }
    }

    void GraphicsSynthesizer::get_resolution(int& w, int& h)
    {
        reg.get_resolution(w, h);
    }

    void GraphicsSynthesizer::get_inner_resolution(int& w, int& h)
    {
        reg.get_inner_resolution(w, h);
    }

    void GraphicsSynthesizer::write64(uint32_t addr, uint64_t value)
    {
        fifo::payload payload;
        payload.write64_payload = { addr, value };

        gs_thread.send_message({ fifo::command::write64_t, payload });

        //Check for interrupt pre-processing
        reg.write64(addr, value);

        //We need a check for SIGNAL here so that we can fire the interrupt as soon as possible
        if (addr == 0x60)
        {
            if (reg.CSR.SIGNAL_generated)
            {
                if (!reg.IMR.signal)
                    intc->assert_IRQ((int)Interrupt::GS);
                else
                    reg.CSR.SIGNAL_irq_pending = true;
            }
        }
    }

    void GraphicsSynthesizer::write64_privileged(uint32_t addr, uint64_t value)
    {
        fifo::payload payload;
        payload.write64_payload = { addr, value };

        gs_thread.send_message({ fifo::command::write64_privileged_t,payload });

        bool old_IMR = reg.IMR.signal;
        reg.write64_privileged(addr, value);

        //When SIGNAL is written to twice, the interrupt will not be processed until IMR.signal is flipped from 1 to 0.
        if (old_IMR && !reg.IMR.signal && reg.CSR.SIGNAL_irq_pending)
        {
            intc->assert_IRQ((int)Interrupt::GS);
            reg.CSR.SIGNAL_irq_pending = false;
        }
    }

    void GraphicsSynthesizer::write32_privileged(uint32_t addr, uint32_t value)
    {
        fifo::payload payload;
        payload.write32_payload = { addr, value };

        gs_thread.send_message({ fifo::command::write32_privileged_t,payload });

        bool old_IMR = reg.IMR.signal;
        reg.write32_privileged(addr, value);
        //When SIGNAL is written to twice, the interrupt will not be processed until IMR.signal is flipped from 1 to 0.
        if (old_IMR && !reg.IMR.signal && reg.CSR.SIGNAL_irq_pending)
        {
            intc->assert_IRQ((int)Interrupt::GS);
            reg.CSR.SIGNAL_irq_pending = false;
        }
    }

    uint32_t GraphicsSynthesizer::get_busdir()
    {
        return reg.BUSDIR;
    }

    uint32_t GraphicsSynthesizer::read32_privileged(uint32_t addr)
    {
        return reg.read32_privileged(addr);
    }

    uint64_t GraphicsSynthesizer::read64_privileged(uint32_t addr)
    {
        return reg.read64_privileged(addr);
    }

    void GraphicsSynthesizer::set_RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a, float q)
    {
        fifo::payload payload;
        payload.rgba_payload = { r, g, b, a, q };

        gs_thread.send_message({ fifo::command::set_rgba_t, payload });
    }

    void GraphicsSynthesizer::set_ST(uint32_t s, uint32_t t)
    {
        fifo::payload payload;
        payload.st_payload = { s, t };

        gs_thread.send_message({ fifo::command::set_st_t, payload });
    }

    void GraphicsSynthesizer::set_UV(uint16_t u, uint16_t v)
    {
        fifo::payload payload;
        payload.uv_payload = { u, v };

        gs_thread.send_message({ fifo::command::set_uv_t, payload });
    }

    void GraphicsSynthesizer::set_XYZ(uint32_t x, uint32_t y, uint32_t z, bool drawing_kick)
    {
        fifo::payload payload;
        payload.xyz_payload = { x, y, z, drawing_kick };

        gs_thread.send_message({ fifo::command::set_xyz_t, payload });
    }

    void GraphicsSynthesizer::set_XYZF(uint32_t x, uint32_t y, uint32_t z, uint8_t fog, bool drawing_kick)
    {
        fifo::payload payload;
        payload.xyzf_payload = { x, y, z, fog, drawing_kick };

        gs_thread.send_message({ fifo::command::set_xyzf_t, payload });
    }

    void GraphicsSynthesizer::load_state(std::ifstream& state)
    {
        fifo::payload payload;
        payload.load_state_payload = { &state };
        gs_thread.send_message({ fifo::command::load_state_t, payload });
        gs_thread.wake_thread();
        fifo::return_message data;
        gs_thread.wait_for_return(fifo::return_command::load_state_done_t, data);

        state.read((char*)&reg, sizeof(reg));
    }

    void GraphicsSynthesizer::save_state(std::ofstream& state)
    {
        fifo::payload payload;
        payload.save_state_payload = { &state };

        gs_thread.send_message({ fifo::command::save_state_t, payload });
        gs_thread.wake_thread();
        fifo::return_message data;
        gs_thread.wait_for_return(fifo::return_command::save_state_done_t, data);

        state.write((char*)&reg, sizeof(reg));
    }

    void GraphicsSynthesizer::send_dump_request()
    {
        fifo::payload p;
        p.no_payload = { 0 };

        gs_thread.send_message({ fifo::command::gsdump_t, p });
        gs_thread.wake_thread();
    }

    void GraphicsSynthesizer::send_message(fifo::message message)
    {
        gs_thread.send_message(message);
    }

    void GraphicsSynthesizer::wake_gs_thread()
    {
        gs_thread.wake_thread();
    }

    std::tuple<uint128_t, uint32_t>GraphicsSynthesizer::request_gs_download()
    {
        fifo::payload payload;
        payload.no_payload = {};
        gs_thread.send_message({ fifo::command::request_local_host_tx, payload });
        gs_thread.wake_thread();
        fifo::return_message data;
        gs_thread.wait_for_return(fifo::return_command::local_host_transfer, data);
        return std::make_tuple(data.payload.data_payload.quad_data, data.payload.data_payload.status);
    }
}

