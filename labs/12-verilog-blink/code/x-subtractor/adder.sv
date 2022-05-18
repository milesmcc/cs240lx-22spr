module adder
    (
        input logic [31:0] a, b,
        output logic [31:0] sum
    );
    /* verilator lint_off UNOPTFLAT */
    logic [32:0] carry;
    half_adder initial_adder (.a(a[0]), .b(b[0]), .s(sum[0]), .c(carry[1]));
    generate
        genvar i;
        for (i = 1; i < 32; i = i + 1)
            full_adder gen_adder (.a(a[i]), .b(b[i]), .cin(carry[i]), .s(sum[i]), .cout(carry[i+1]));
    endgenerate
endmodule

module subtractor
    (
        input logic [31:0] a, b,
        output logic [31:0] diff
    );

    logic [31:0] complement;
    assign complement = ~b + 1;
    adder _x (.a, .b(complement), .sum(diff));
endmodule

module half_adder
    (
        input logic a, b,
        output logic c, s
    );
    assign s = a ^ b;
    assign c = a && b;
endmodule

module full_adder
    (
        input logic a, b, cin,
        output logic cout, s
    );
    logic is_one, is_two, is_three;

    assign is_one = (a && !(b || cin)) || (b && !(a || cin)) || (cin && !(a || b));
    assign is_two = (a && b && !cin) || (a && !b && cin) || (!a && b && cin);
    assign is_three = (a && b && cin);
    
    assign s = is_one || is_three;
    assign cout = is_two || is_three;
endmodule
