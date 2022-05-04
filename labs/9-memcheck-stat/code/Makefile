# list out the driver program source 
# TEST_SRC = tests-2.0/part2-test6-no-leak.c tests-2.0/part2-test1-no-leak.c tests-2.0/part2-test2-no-leak.c tests-2.0/part2-test4-no-leak.c tests-2.0/part2-test5-gc.c
# TEST_SRC = tests-2.0/part1-test4-bug.c tests-2.0/part1-test1-no-bug.c tests-2.0/part1-test2-bug.c tests-2.0/part1-test3-bug.c tests-2.0/part1-test6-bugs.c tests-2.0/part1-test5-no-bug.c
TEST_SRC = tests-stat/part3-test1.c tests-stat/part3-test2.c tests-stat/part3-test3.c

#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
SUPPORT_OBJS := ckalloc.o ck-memcheck.o kr-malloc.o ck-gc.o gc-asm.o interrupts-asm.o interrupts-c.o timer-interrupt.o

# define this if you need to give the device for your pi
TTYUSB = 

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

CFLAGS_EXTRA = -fno-toplevel-reorder -fno-omit-frame-pointer -mpoke-function-name -fsanitize=kernel-address


DEPS = ./Makefile
include $(CS240LX_2022_PATH)/libpi/mk/Makefile.template

checkopt:
	@echo "going to check opt level: -O:"
	@make -s clean && make -s "OPT_LEVEL=-O" RUN=0
	@make check
	
	@echo "going to check opt level: -O2:"
	@make -s clean && make -s "OPT_LEVEL=-O2" RUN=0
	@make check

	@echo "going to check opt level: -O3:"
	@make -s clean && make -s "OPT_LEVEL=-O3" RUN=0
	@make check

	@echo "going to check opt level: -Ofast:"
	@make -s clean && make -s "OPT_LEVEL=-Ofast" RUN=0
	@make check