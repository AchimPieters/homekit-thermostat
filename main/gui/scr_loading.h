typedef void (*on_wifi_reconnect)(void);

void gui_loading_scr();
void gui_loading_show_reconnect_btn(on_wifi_reconnect btn_cb);
void gui_loading_add_log(const char *msg);