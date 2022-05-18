// Output q will count up to M and then reset to 0.
module counter
    #(
        parameter M = 10
    )
    (
        input  logic clk, rst,
        output logic [$clog2(M)-1:0] q
    );
    logic [$clog2(M)-1:0] count;

    always_ff @(posedge clk, posedge rst)
        if (rst)
            count <= '0;
        else if (count == M)
            count <= '0;
        else
            count <= (count + 1);

    assign q = count;
endmodule
