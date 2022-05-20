module uart_tx
    (
        input logic clk, rst,
        input logic tx_start,      // pulse one tick when transmission should begin
        input logic tick,          // baud rate oversampled tick
        input logic [7:0] din,     // data to send (user must keep data valid
                                   // at least until tx_done_tick is asserted)
        output logic tx_done_tick, // pulse one tick when done
        output logic tx            // serial data
    );

    /* verilator public_module */

    typedef enum {idle, start, data, stop} state_t;
    state_t state_reg = idle;
    state_t state_next;

    logic [2:0] index_reg, index_next;
    logic [3:0] tick_reg, tick_next;
    logic tx_done_tick_next, tx_next;

    always_ff @(posedge clk, posedge rst) begin
        if (rst) begin
            state_reg <= idle;
        end else begin
            state_reg <= state_next;
            tx <= tx_next;
            index_reg <= index_next;
            tick_reg <= tick_next;
            tx_done_tick <= tx_done_tick_next;
        end
    end

    always_comb begin
        state_next = state_reg;
        index_next = index_reg;
        tick_next = tick_reg;
        tx_done_tick_next = 0;
        tx_next = tx;

        if (tick) begin
            tick_next = tick_reg + 1;

            unique case (state_reg)
                idle: begin
                    if (tx_start) begin
                        tick_next = '0;
                        state_next = start;
                        index_next = '0;
                    end
                end
                start: begin
                    if(tick_reg == 15)
                        state_next = data;
                end
                data: begin
                    if(tick_reg == 15) begin
                        if (index_reg == 7)
                            state_next = stop;
                        else
                            index_next = index_reg + 1;
                    end
                end
                stop: begin
                    if (tick_reg == 15)
                        state_next = idle;
                end
            endcase
        end

        unique case (state_reg)
            idle: begin
                tx_next = 1;
            end
            start: begin
                tx_next = 0;
            end
            data: begin
                tx_next = din[index_reg];
            end
            stop: begin
                tx_next = 1;
                tx_done_tick_next = 1;
            end
        endcase
    end
endmodule
