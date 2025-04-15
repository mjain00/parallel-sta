`default_nettype none

module bigadder (
    input logic [15:0] a, b,
    output logic [16:0] x,
    input logic clk
);

    logic [15:0] a_, b_;

    always_ff @(posedge clk) begin
        a_ <= a;
        b_ <= b;

        x <= a_ + b_;
    end

endmodule
