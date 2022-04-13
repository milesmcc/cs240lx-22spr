// very dumb starter code.  you should rewrite and customize.
//
// when done i would suggest pulling it out into an device source/header 
// file and put in libpi/src so can use later.

#include "rpi.h"
#include "rpi-interrupts.h"

enum { ir_eps = 200 };

#define IN_PIN 21
#define OUT_PIN 5
#define TRANSMIT_USEC (1000000 / (38000 * 2)) 

// we should never get this.
enum { NOISE = 0 } ;

typedef struct readings { unsigned usec, v; } reading;

const char *key_to_str(unsigned x) {
    switch (x) {
        case 0xc3de23dc: return "PLUS";
        case 0xc3dfc23c: return "MINUS";
        case 0xc3de5da2: return "REWIND";
        case 0xc3df5ca2: return "PLAY/PAUSE";
        case 0xc3df9c62: return "FORWARD";
        case 0xc3dedd22: return "REPEAT";
        case 0xc3dee31c: return "MUTE";
        case 0xc3dea35c: return "SOURCE";
        case 0xc3dfe21c: return "POWER";
        default: return 0;
    }
}

static void transmit_for(int pin, unsigned time) {
    unsigned rb = timer_get_usec();
    while (1) {
        gpio_set_on(pin);
        delay_us(TRANSMIT_USEC);
        gpio_set_off(pin);
        delay_us(TRANSMIT_USEC);
        
        unsigned ra = timer_get_usec();
        if ((ra - rb) >= time) {
            break;
        }
    }
}

static void transmit_value(int pin, unsigned value) {
    // Send header
    transmit_for(pin, 4500);
    delay_us(4500);

    // Start sending the data
    for(int i = 31; i >= 0; i--) {
        int val = (value >> i) & 1;
        if (val) {
            transmit_for(pin, 1600);
        } else {
            transmit_for(pin, 600);
        }
        delay_us(600);
    }
}

// adapt your read_while_equal: return 0 if timeout passed, otherwise
// the number of microseconds + 1 (to prevent 0).
static int read_while_eq(int pin, int v, unsigned timeout) {
    unsigned rb = timer_get_usec();
    while (1) {
        unsigned ra = timer_get_usec();
        if ((ra - rb) >= timeout) {
            return 0;
        }
        if (gpio_read(IN_PIN) != v) {
            return ra - rb;
        }
    }
}

// integer absolute value.
static int abs(int x) {
    return x < 0 ? -x : x; 
}

// return 0 if e is closer to <lb>, 1 if its closer to <ub>
static int pick(struct readings *e, unsigned lb, unsigned ub) {
    assert(e->v);
    int len = e->usec;
    unsigned lb_len = abs((int)lb - len);
    unsigned ub_len = abs((int)ub - len);
    return ub_len < lb_len ? 0 : 1;

    // panic("invalid time: <%d> expected %d or %d\n", e->usec, lb, ub);
}

// return 1 if is a skip: skip = delay of 550-/+eps
static int is_skip(struct readings *e) {
    return (abs((int)e->usec - (int)600) < 200) && e->v == 0;
}

// header is a delay of 9000 and then a delay of 4500
int is_header(struct readings *r, unsigned n) {
    return (r[n].usec > 3750) && (r[n].v == 0) && (r[n+1].usec > 3750) && (r[n+1].v == 1);
}

int find_header(struct readings *r, unsigned n) {
    if (n < 4) {return 0;}
    int start = 0;
    for(int i = 0; i < n - 1; i++) {
        if(is_header(r, i)) {
            start = i + 3;
        }
    }
    return start;
}

static void print_readings(struct readings *r, int n) {
    assert(n);
    printk("-------------------------------------------------------\n");
    for(int i = 0; i < n; i++) {
        if(i) 
            assert(!is_header(r+i,n-i));
        printk("\t%d: %d = %d usec\n", i, r[i].v, r[i].usec);
    }
    printk("readings=%d\n", n);
    // if(!is_header(r,n))
    //     printk("NOISE\n");
    // else
    //     printk("convert=%x\n", convert(r,n));
}

// convert <r> into an integer by or'ing in 0 or 1 depending on the 
// time value.
//
// assert that they are seperated by skips!
unsigned convert(struct readings *r, unsigned n) {
    printk("converting %d readings...\n");
    int start = find_header(r, n);
    assert(start > 0);

    print_readings(r, (int)n);

    printk("converting n = %d\n", n - start);
    uint32_t val = 0;
    for(int i = start; i < n; i += 2) {
        if (is_header(r, i)) {
            break;
        }

        val |= pick(&r[i], 600, 1600);
        val <<= 1;
        // printk("    %d = %d for %d\n", i, r[i].v, r[i].usec);
        if (i < (n - 1)) {
            // printk("    %d = %d for %d\n", i+1, r[i+1].v, r[i+1].usec);
            printk("%d => %d for %d\n", i + 1, r[i + 1].v, r[i+1].usec);
            assert(is_skip(&r[i + 1]));
        }
    }
    return val;
}

// read in values until we get a timeout, return the number of readings.  
static int get_readings(int in, struct readings *r, unsigned N) {
    int n = 0;
    while(n < N) {
        int val = gpio_read(in);
        int dur = read_while_eq(in, val, 100000);
        if (dur == 0) {
            return n;
        } else {
            r[n].usec = dur;
            r[n].v = val;
        }
        n++;
    }
    return n;
}

// initialize the pin.
int tsop_init(int input) {
    // is open hi or lo?  have to set pullup or pulldown
    gpio_set_input(IN_PIN);
    gpio_set_pullup(IN_PIN);
    return 1;
}

static unsigned int_readings_n;
static reading int_readings[256];

void interrupt_vector(unsigned pc) {
    static unsigned last_time = 0;
    static unsigned last_val = 0;
    
    unsigned now = timer_get_usec();
    unsigned val = gpio_read(IN_PIN);
    gpio_event_clear(IN_PIN);

    if (last_time == 0) {
        // Exit early on first invocation
        last_time = now;
        last_val = val;
        return;
    }

    if(val != last_val) {
        unsigned time_diff = now - last_time;
        int_readings[int_readings_n].usec = time_diff;
        int_readings[int_readings_n].v = !last_val;
        int_readings_n++;

        last_time = now;
        last_val = val;
    }
}

void sync_read(void) {
    int in = IN_PIN;
    tsop_init(in);

    output("about to start reading\n");

    // very dumb starter code
    while(1) {
        // wait until signal: or should we timeout?
        while(gpio_read(in)) {}
#       define N 256
        static struct readings r[N];
        int n = get_readings(in, r, N);

        output("done getting readings\n");
    
        unsigned x = convert(r,n);
        output("converted to %x\n", x);
        const char *key = key_to_str(x);
        if(key)
            printk("%s\n", key);
        else
            // failed: dump out the data so we can figure out what happened.
            // print_readings(r,n);
            {}
    }
	printk("stopping ir send/rec!\n");
}

void interrupt_read() {
    int in = 21;
    tsop_init(in);
    
    int_init();
    gpio_int_rising_edge(in);
    gpio_int_falling_edge(in);
    system_enable_interrupts();

    gpio_set_output(OUT_PIN);

    output("about to start reading\n");

    // very dumb starter code
    while(1) {
        int_readings_n = 0;
        delay_ms(1000);

        transmit_value(OUT_PIN, 0xdeadbeef);
    
        if(!find_header(int_readings, int_readings_n)) {
            output(".");
            continue;
        } else {
            output(" found header!\n");
        }
        unsigned x = ~(convert(int_readings, int_readings_n) >> 1);
        output("converted to %x\n", x);
        const char *key = key_to_str(x);
        if(key)
            printk("%s\n", key);
        else
            // failed: dump out the data so we can figure out what happened.
            // print_readings(r,n);
            {}
    }
	printk("stopping ir send/rec!\n");
}

void sync_write() {
    gpio_set_output(OUT_PIN);

    for(int i = 0; i < 6400; i++) {
        unsigned to_transmit = i * 1047;
        printk("Transmitting %x...\n", to_transmit);
        transmit_value(OUT_PIN, i);
        delay_ms(200);
    } 
}

void notmain(void) {
    // sync_write();
    // sync_read();
    interrupt_read();
    clean_reboot();
}
