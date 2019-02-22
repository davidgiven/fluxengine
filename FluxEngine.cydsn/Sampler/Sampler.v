
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 02/21/2019 at 23:51
// Component: Sampler
module Sampler (
	output interrupt,
	output reg [7:0] result,
	input clock,
	input sample
);

//`#start body` -- edit after this line, do not edit this line

reg [6:0] counter;
reg [6:0] lastpulse;
wire [6:0] interval = counter - lastpulse;
wire interval_is_zero = (interval == 0);

assign interrupt = sample | interval_is_zero;

always @(posedge clock)
begin
    counter <= counter + 1;
end

always @(posedge interrupt)
begin
    result[6:0] <= interval[6:0];
    result[7] <= interval_is_zero;
    lastpulse <= counter;
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
