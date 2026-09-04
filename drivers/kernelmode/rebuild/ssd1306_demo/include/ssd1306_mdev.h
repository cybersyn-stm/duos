#ifndef SSD1306_MDEV_H
#define SSD1306_MDEV_H

struct ssd1306_data; // 前向声明
int ssd1306_misc_register(struct ssd1306_data *data);
void ssd1306_misc_deregister(struct ssd1306_data *data);

#endif /* SSD1306_MDEV_H */
