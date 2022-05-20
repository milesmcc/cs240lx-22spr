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
    
    logic [4:0] dout_index_reg;
    logic [4:0] dout_index_next;
    logic [11:0] ticks;

    always_ff @(posedge clk, posedge rst)
        if (rst) begin
            state_reg <= idle;
            ticks <= '0;
            rx_done_tick <= '0;
        end else begin
            state_reg <= state_next;
            dout_index_reg <= dout_index_next;
            dout_reg <= dout_next;
            if (state_next == idle) begin
                ticks <= '0;
                dout_index_reg <= '0;
                rx_done_tick <= '0;
            end else if (state_next == data) begin
                
            end
            if (state_next == stop) begin
                dout <= dout_next;
                rx_done_tick <= '1;
            end
            if (tick)
                ticks <= ticks + 1;
        end

    always_comb begin
        state_next = state_reg;
        dout_next = dout_reg;
        dout_index_next = dout_index_reg;

        if (tick) begin
            unique case (state_reg)
                idle: begin
                    if (!rx)
                        state_next = start;
                end
                start: begin
                    if (ticks > 16) begin
                        state_next = data;
                        dout_next = 0;
                    end
                end
                data: begin
                    if (ticks % 16 == 8) begin
                        dout_next = {rx,dout_reg[7:1]};
                        dout_index_next = dout_index_reg + 1;
                        if (dout_index_next == 8)
                            state_next = stop;
                    end
                end
                stop: begin
                    if (ticks % 16 == 7) begin
                        state_next = idle;
                    end
                end
            endcase
        end
    end
endmodule
