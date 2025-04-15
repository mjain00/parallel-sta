`default_nettype none

module simple (
    input logic a, b,
    output logic x,
    input logic clk
);

    logic a_, b_;

    always_ff @(posedge clk) begin
        a_ <= a;
        b_ <= b;
        x <= a_ ^ b_;
    end

endmodule
