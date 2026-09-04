#ifndef NIXIE_TUBE
#define NIXIE_TUBE

#define CHIP "/dev/	gpiochip0"
#define SER 21
#define RCLK 20
#define SRCLK 19
#define SRCLK_CLEAN 18

unsigned char number[10] = {0x09, 0x02, 0x03, 0x07, 0x06,
                            0x04, 0x05, 0x01, 0x00, 0x08};

struct gpiod_chip *chip;
struct gpiod_line *ser, *rclk, *srclk, *srclk_clean;

#endif
