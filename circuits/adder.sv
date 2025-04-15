`default_nettype none

module adder (
    input logic [2:0] a, b,
    output logic [3:0] x,
    input logic clk
);

    logic [2:0] a_, b_;

    always_ff @(posedge clk) begin
        a_ <= a;
        b_ <= b;

        x <= a_ + b_;
    end

endmodule
