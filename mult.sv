`default_nettype none

module mult (
    input logic [3:0] a, b,
    output logic [7:0] x,
    input logic clk
);

    logic [3:0] a_, b_;

    always_ff @(posedge clk) begin
        a_ <= a;
        b_ <= b;

        x <= a_ * b_;
    end

endmodule
