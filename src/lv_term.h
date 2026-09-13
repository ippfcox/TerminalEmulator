#ifndef LV_TERM_H__
#define LV_TERM_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "lvgl.h"

    lv_obj_t *lv_term_create(lv_obj_t *parent);
    void lv_term_set_font(lv_obj_t *term, const char *ttf_path);
    void lv_term_set_size(lv_obj_t *term, int cols, int rows);
    void lv_term_feed_input(lv_obj_t *term, const char *data, size_t len);
    void lv_term_handle_key(lv_obj_t *term, uint32_t keysym, uint32_t ascii,
        unsigned int mods, uint32_t unicode);
    void lv_term_handle_mouse(lv_obj_t *term,
        unsigned int cells_x, unsigned int cell_y,
        unsigned int pixel_x, unsigned pixel_y,
        unsigned int button, unsigned int event, unsigned char flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LV_TERM_H__