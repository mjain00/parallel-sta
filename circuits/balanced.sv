`default_nettype none

module balanced (
    input logic [7:0] a,
    output logic x,
    input logic clk
);

    logic [7:0] a_;

    always_ff @(posedge clk) begin
        a_ <= a;

        // AND-reduce (in a balanced tree)
        x <= ((a_[0] & a_[1]) & (a_[2] & a_[3])) & ((a_[4] & a_[5]) & (a_[6] & a_[7]));
    end

endmodule
