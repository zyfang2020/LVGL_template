#include "lvgl.h"
#include "user_lv.h"
void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    } 
}
 void hello_world(void)
 {
    lv_obj_t * lable;
    lv_obj_t * button = lv_button_create(lv_scr_act());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);
    lable = lv_label_create(button);
     lv_obj_add_event_cb(button, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/
    lv_obj_align(button,LV_ALIGN_CENTER,0,0);
    lv_label_set_text(lable, "Hello World!");
 }