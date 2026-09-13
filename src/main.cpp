#include <spdlog/spdlog.h>
#include <lvgl.h>
#include <chrono>
#include <thread>
// #include "libtsm.h"

int main()
{
    lv_init();

    // lv_tick_set_cb([] {
    //     return static_cast<uint32_t>(
    //         std::chrono::duration_cast<std::chrono::milliseconds>(
    //             std::chrono::steady_clock::now().time_since_epoch())
    //             .count());
    // });

    lv_display_t *disp = lv_x11_window_create("Terminal", 800, 600);
    if (!disp)
    {
        SPDLOG_ERROR("lv_x11_window_create failed");
        return 1;
    }

    lv_x11_inputs_create(disp, nullptr);

    while (true)
    {
        lv_timer_handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    lv_deinit();

    return 0;
}