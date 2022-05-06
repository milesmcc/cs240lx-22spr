#include <stdbool.h>

#include "gpio.h"
#include "rpi.h"

#define SCL_PIN 3
#define SDA_PIN 2

void I2C_delay(void) {
    delay_us(5);
}

bool scl_val = false;
bool sda_output = false;

bool read_SCL(void) {  // Return current level of SCL line, 0 or 1
    return scl_val;
}

bool read_SDA(void) {  // Return current level of SDA line, 0 or 1
    if (sda_output) {
        gpio_set_input(SDA_PIN);
        gpio_set_pulldown(SDA_PIN);
        sda_output = false;
    }
    
    char val = gpio_read(SDA_PIN);
    // printk("sda = %d\n", val);
    return val;
}

void set_SCL(void) {  // Do not drive SCL (set pin high-impedance)
    gpio_set_off(SCL_PIN);
    scl_val = 1;
}

void clear_SCL(void) {  // Actively drive SCL signal low
    gpio_set_on(SCL_PIN);
    scl_val = 0;
}

void set_SDA(void) {  // Do not drive SDA (set pin high-impedance)
    if (!sda_output) {
        gpio_set_output(SDA_PIN);
        sda_output = true;
    }

    gpio_set_off(SDA_PIN);
}

void clear_SDA(void) {  // Actively drive SDA signal low
    if (!sda_output) {
        gpio_set_output(SDA_PIN);
        sda_output = true;
    }

    gpio_set_on(SDA_PIN);
}

void arbitration_lost(void) {
    // pass...? TODO
}

bool started = false;  // global data

void i2c_start_cond(void) {
    if (started) {
        // if started, do a restart condition
        // set SDA to 1
        set_SDA();
        I2C_delay();
        set_SCL();
        while (read_SCL() == 0) {  // Clock stretching
            // You should add timeout to this loop (me: no...)
        }

        // Repeated start setup time, minimum 4.7us
        I2C_delay();
    }

    // if (read_SDA() == 0) {
    //     arbitration_lost();
    // }

    // SCL is high, set SDA from 1 to 0.
    clear_SDA();
    I2C_delay();
    clear_SCL();
    started = true;
}

void i2c_init() {
    gpio_set_output(SCL_PIN);
}

void i2c_stop_cond(void) {
    // set SDA to 0
    clear_SDA();
    I2C_delay();

    set_SCL();
    // Clock stretching
    while (read_SCL() == 0) {
        // add timeout to this loop.
    }

    // Stop bit setup time, minimum 4us
    I2C_delay();

    // SCL is high, set SDA from 0 to 1
    set_SDA();
    I2C_delay();

    // if (read_SDA() == 0) {
    //     arbitration_lost();
    // }

    started = false;
}

// Write a bit to I2C bus
void i2c_write_bit(bool bit) {
    if (bit) {
        set_SDA();
    } else {
        clear_SDA();
    }

    // SDA change propagation delay
    I2C_delay();

    // Set SCL high to indicate a new valid SDA value is available
    set_SCL();

    // Wait for SDA value to be read by target, minimum of 4us for standard mode
    I2C_delay();

    while (read_SCL() == 0) {  // Clock stretching
                               // You should add timeout to this loop
    }

    // SCL is high, now data is valid
    // If SDA is high, check that nobody else is driving SDA
    // if (bit && (read_SDA() == 0)) {
    //     arbitration_lost();
    // }

    // Clear the SCL to low in preparation for next change
    clear_SCL();
}

// Read a bit from I2C bus
bool i2c_read_bit(void) {
    bool bit;

    // Let the target drive data
    set_SDA();

    // Wait for SDA value to be written by target, minimum of 4us for standard
    // mode
    I2C_delay();

    // Set SCL high to indicate a new valid SDA value is available
    set_SCL();

    while (read_SCL() == 0) {  // Clock stretching
                               // You should add timeout to this loop
    }

    // Wait for SDA value to be written by target, minimum of 4us for standard
    // mode
    I2C_delay();

    // SCL is high, read out bit
    bit = read_SDA();

    // Set SCL low in preparation for next operation
    clear_SCL();

    return bit;
}

// Write a byte to I2C bus. Return 0 if ack by the target.
bool i2c_write_byte(bool send_start, bool send_stop, unsigned char byte) {
    unsigned bit;
    bool nack;

    if (send_start) {
        i2c_start_cond();
    }

    for (bit = 0; bit < 8; ++bit) {
        i2c_write_bit((byte & 0x80) != 0);
        byte <<= 1;
    }

    nack = i2c_read_bit();

    if (send_stop) {
        i2c_stop_cond();
    }

    return nack;
}

// Read a byte from I2C bus
unsigned char i2c_read_byte(bool nack, bool send_stop) {
    unsigned char byte = 0;
    unsigned char bit;

    for (bit = 0; bit < 8; ++bit) {
        byte = (byte << 1) | i2c_read_bit();
    }

    i2c_write_bit(nack);

    if (send_stop) {
        i2c_stop_cond();
    }

    return byte;
}

// write <nbytes> of <datea> to i2c device address <addr>
int i2c_write(unsigned addr, uint8_t data[], unsigned nbytes) {
    i2c_write_byte(1, 0, addr);

    for (int i = 0; i < nbytes; i++) {
        i2c_write_byte(0, i == nbytes - 1, data[i]);
    }
    return nbytes;
}

// read <nbytes> of <datea> from i2c device address <addr>
int i2c_read(unsigned addr, uint8_t data[], unsigned nbytes) {
    i2c_write_byte(true, true, addr | 1);

    I2C_delay();

    for (int i = 0; i < nbytes; i++) {
        data[i] = i2c_read_byte(0, i == nbytes - 1);
    }
    return nbytes;
}