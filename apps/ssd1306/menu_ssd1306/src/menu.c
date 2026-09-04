#include "linklisted.h"
#include "ssd1306.h"
#include "ssd1306_8x16.h"
#include <string.h>

buff_type file_buffer[FB_SIZE];

linklisted_t *menu_create() {
    linklisted_t *menu = linkedlist_create("Setting", NULL);
    linklisted_add(menu, "WiFi", NULL);
    linklisted_add(menu, "Menu-1", NULL);
    linklisted_add(menu, "Menu-2", NULL);
    return menu;
}
// set str in display mind
int str_mind(char *str) {
    int mind_retrun = 0;
    int str_long_size = strlen(str);

    mind_retrun = SSD1306_W - (str_long_size * 8);
    mind_retrun = mind_retrun / 2;
    return mind_retrun;
}

int main() {
    linklisted_t *menu = menu_create();
    int page = 0;
    int x = 0;
    while (1) {
        ssd1306_buffer_clear(file_buffer);

        linklisted_t *current_menu = menu;
        page = 0;
        while (current_menu != NULL) {
            x = str_mind(current_menu->name);
            ssd1306_draw_buffer_string8x16(file_buffer, x, page,
                                           current_menu->name);
            current_menu = current_menu->next;
            page = page + 1;
        }

        send_buffer(file_buffer);
        sleep(1);
    }
}
