#include "output.h"
#include "menu.h"

void menu_send_buffer(UICtx_t *ui, FileOperation_t *file) {
    send_buffer(ui->fb, file->screen_fd);
    ssd1306_buffer_clear(ui->fb);
}
