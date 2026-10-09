#ifndef NIXIE_TUBE
#define NIXIE_TUBE

/* 板上实测 sysfs 全局号 -> /dev/gpiochip0 (porta, 基址 480):
 *   SER  = 506 -> line 26
 *   RCLK = 507 -> line 27
 *   SRCLK= 505 -> line 25
 *   SRCLR= 502 -> line 22
 */
#define CHIP "/dev/gpiochip0"
#define SER 26
#define RCLK 27
#define SRCLK 25
#define SRCLK_CLEAN 22

extern unsigned char number[10];

extern struct gpiod_chip *chip;
extern struct gpiod_line *ser, *rclk, *srclk, *srclk_clean;

#endif
