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

    logic [7:0] dout_next;
    
    logic [2:0] dout_index_reg;
    logic [2:0] dout_index_next;
    
    logic [3:0] ticks_reg;
    logic [3:0] ticks_next;

    logic rx_done_next;

    always_ff @(posedge clk, posedge rst)
        if (rst) begin
            state_reg <= idle;
            rx_done_tick <= '0;
            ticks_reg <= '0;
        end else begin
            state_reg <= state_next;
            dout_index_reg <= dout_index_next;
            dout <= dout_next;
            ticks_reg <= ticks_next;
            rx_done_tick <= rx_done_next;
            dout <= dout_next;
        end

    always_comb begin
        state_next = state_reg;
        dout_next = dout;
        dout_index_next = dout_index_reg;
        rx_done_next = rx_done_tick;
        ticks_next = ticks_reg;

        if (tick) begin
            ticks_next = ticks_reg + 1;

            unique case (state_reg)
                idle: begin
                    if (!rx)
                        state_next = start;
                        ticks_next = '0;
                end
                start: begin
                    if (ticks_reg == 15) begin
                        state_next = data;
                        dout_next = '0;
                        rx_done_next = '0;
                        dout_index_next = '0;
                    end
                end
                data: begin
                    if (ticks_reg == 7) begin
                        dout_next = {rx,dout[7:1]};
                        dout_index_next = dout_index_reg + 1;
                        if (dout_index_reg == 7) begin
                            state_next = stop;
                            rx_done_next = '1;
                        end
                    end
                end
                stop: begin
                    if (ticks_reg == 7) begin
                        state_next = idle;
                    end
                end
            endcase
        end
    end
endmodule
