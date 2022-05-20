module uart_top
    (
        input logic clk,
        input logic rst,

        output logic led_r,
        output logic led_g,
        output logic led_b,

        output logic gpio_28,
        input logic gpio_38,
        output logic gpio_2,
        input logic gpio_46,

        input logic rx,
        output logic tx
    );

    logic tick;

    // baud rate of 19200 oversampled 16x on our CLK_MHZ clock:
    localparam div = (`CLK_MHZ * 1000000) / (120200 * 16);

    counter #(.M(div)) baud_generator (
        .clk, .rst(rst),
        .max_tick(tick)
    );

    logic [7:0] rx_data;
    logic rx_done_tick;

    logic a_full, b_full;
    logic [7:0] rx_data_buf_from_a, rx_data_buf_from_b, rx_a, rx_b;
    logic tx_done_tick_a, tx_done_tick_b, tx_done_tick_monitor;

    always_ff @(posedge clk, posedge rst) begin
        if(!rst) begin
            gpio_28 <= gpio_46;
            gpio_2 <= gpio_38;
        end
    end

    uart_tx tx_unit_to_monitor_a (
        .clk, .rst(rst),
        .tx_start(a_full), .tick,
        .din(rx_data_buf_from_a), .tx_done_tick(tx_done_tick_monitor),
        .tx(tx)
    );

    // uart_tx tx_unit_to_monitor_b (
    //     .clk, .rst(rst),
    //     .tx_start(b_full), .tick,
    //     .din(rx_data_buf_from_b), .tx_done_tick(tx_done_tick_monitor),
    //     .tx(tx)
    // );

    // uart_tx tx_unit_to_a (
    //     .clk, .rst(rst),
    //     .tx_start(b_full), .tick,
    //     .din(rx_data_buf_from_b), .tx_done_tick(tx_done_tick_a),
    //     .tx(gpio_2)
    // );

    // uart_tx tx_unit_to_b (
    //     .clk, .rst(rst),
    //     .tx_start(a_full), .tick,
    //     .din(rx_data_buf_from_a), .tx_done_tick(tx_done_tick_b),
    //     .tx(gpio_28)
    // );

    uart_rx rx_unit_a (
        .clk, .rst(rst),
        .rx(gpio_46), .tick,
        .rx_done_tick, .dout(rx_a)
    );
    flag_buf rx_from_a (
        .clk, .rst(rst),
        .clr_flag(tx_done_tick_monitor), .set_flag(rx_done_tick),
        .din(rx_a),
        .flag(a_full),
        .dout(rx_data_buf_from_a)
    );
    // flag_buf rx_from_b (
    //     .clk, .rst(rst),
    //     .clr_flag(tx_done_tick_b), .set_flag(rx_done_tick),
    //     .din(gpio_38),
    //     .flag(b_full),
    //     .dout(rx_data_buf_from_b)
    // );
endmodule
