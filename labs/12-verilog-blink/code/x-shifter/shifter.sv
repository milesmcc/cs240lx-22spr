module shifter
    #(
        parameter N = 1,
        parameter BITS = 32
    )
    (
        input logic [BITS - 1:0] in,
        output logic [BITS - 1:0] shifted
    );
    /* verilator lint_off UNOPTFLAT */
    generate
        genvar i;
        for (i = 0; i < BITS; i = i + 1)
            assign shifted[i + N] = in[i];
    endgenerate
endmodule
