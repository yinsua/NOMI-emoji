#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"
#include "nomi_eyes.h"

int main(int argc, char ** argv)
{
    lv_init();

    lv_display_t * display = lv_windows_create_display(
        L"NOMI Eye Simulator - 240x240",
        240,
        240,
        200,
        false,
        true);
    if(!display) return 1;

    HWND window = lv_windows_get_display_window_handle(display);
    uint32_t init_deadline = GetTickCount() + 1000U;
    while(!lv_display_get_buf_active(display) && IsWindow(window)) {
        lv_timer_handler();
        if((int32_t)(GetTickCount() - init_deadline) >= 0) return 2;
        Sleep(5);
    }

    lv_lock();

    if(!lv_windows_acquire_pointer_indev(display)) {
        lv_unlock();
        return 3;
    }
    nomi_eyes_create(lv_screen_active());
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--fast") == 0) nomi_eyes_set_carousel_period(700);
    }
    lv_obj_invalidate(lv_screen_active());

    lv_unlock();

    nomi_expression_t shown_expression = NOMI_EXPRESSION_COUNT;
    lv_refr_now(display);
    while(IsWindow(window)) {
        uint32_t wait_ms = lv_timer_handler();
        nomi_expression_t current_expression = nomi_eyes_current_expression();
        if(current_expression != shown_expression) {
            char title[96];
            snprintf(title, sizeof(title), "NOMI Eye Simulator - %02d/%02d %s",
                     (int)current_expression + 1,
                     (int)NOMI_EXPRESSION_COUNT,
                     nomi_eyes_expression_name(current_expression));
            SetWindowTextA(window, title);
            shown_expression = current_expression;
        }
        if(wait_ms == LV_NO_TIMER_READY || wait_ms > 20) wait_ms = 20;
        if(wait_ms == 0) wait_ms = 1;
        Sleep(wait_ms);
    }

    lv_deinit();
    return 0;
}
