// Count up to M and then assert max_tick for one cycle and reset
module counter
    #(
        parameter M = 10
    )
    (
        input  logic clk, rst,
        output logic max_tick
    );
    logic [($clog2(M)+1)-1:0] count;

    always_ff @(posedge clk, posedge rst)
        if (rst)
            count <= '0;
        else if (count >= M)
            count <= '0;
        else
            count <= (count + 1);

    assign max_tick = count == M - 1;
endmodule