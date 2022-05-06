# we give you  .o's to start with: swap yours in.
TEST_SRC = game.c

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
SRC := ads1115.c neopixel.c
STAFF_OBJS := staff-i2c.o

# define this if you need to give the device for your pi
TTYUSB = /dev/cu.usbserial-0001
BOOTLOADER = pi-install

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
SUPPORT_OBJS := $(SRC:.c=.o)
include $(CS240LX_2022_PATH)/libpi/mk/Makefile.template
