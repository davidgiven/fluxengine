
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 02/21/2019 at 23:51
// Component: Sampler
module Sampler (
	output reg interrupt,
	output reg [7:0] result,
	input clock,
	input sample
);

//`#start body` -- edit after this line, do not edit this line

reg [6:0] counter;   // monotonically increasing counter

reg last_sample;

always @(posedge clock)
begin
    if (counter == 0)
    begin
        // Rollover.
        result = 8'h80;
        interrupt = 1;
        counter = 1;
    end
    else if (sample && !last_sample)
    begin
        // A sample happened since the last clock.
        result[6:0] = counter;
        result[7] = 0;
        interrupt = 1;
        counter = 1;
    end
    else
    begin
        result = 0;
        counter = counter + 1;
        interrupt = 0;
    end
    last_sample = sample;
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
