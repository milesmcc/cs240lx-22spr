module uart_rx
    (
        input logic clk, rst,
        input logic rx,            // serial data
        input logic tick,          // baud rate oversampled tick
        output logic rx_done_tick, // pulse one tick when done
        output logic [7:0] dout    // output data
    );

    /* verilator public_module */

    typedef enum {idle, start, data, stop} state_t;
    state_t state_reg = idle;
    state_t state_next;

    logic [7:0] dout_reg;
    logic [7:0] dout_next;
    
    logic [3:0] dout_index;
    logic [11:0] ticks;

    always_ff @(posedge clk, posedge rst)
        if (rst) begin
            state_reg <= idle;
            ticks <= '0;
        end else begin
            state_reg <= state_next;
            if (state_next == stop || state_next == idle) begin
                ticks <= '0;
                dout_index <= '0;
            end
            if (state_next == stop)
                dout <= dout_next;
            if (tick)
                ticks <= ticks + 1;
        end

    always_comb begin
        state_next = state_reg;
        if (tick) begin
            unique case (state_reg)
                idle: begin
                    if (!rx)
                        state_next = start;
                end
                start: begin
                    if (ticks >= 24) begin
                        state_next = data;
                        dout_next = 0;
                    end
                end
                data: begin
                    if (ticks % 16 == 8) begin
                        dout_next = dout_next | (1 << dout_index);
                    end
                end
                stop: begin
                    state_next = idle;
                end
            endcase
        end
    end
endmodule
