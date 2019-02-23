
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 02/22/2019 at 20:31
// Component: Sequencer

module Sequencer (
	output reg interrupt,
	output reg wdata,
	input clock,
	input [7:0] data,
    input reset
);

//`#start body` -- edit after this line, do not edit this line

reg [6:0] counter;

always @(posedge clock)
begin
    if (reset)
    begin
        counter <= 0;
        interrupt <= 0;
        wdata <= 0;
    end
    else
    begin
        if (counter == data[6:0])
        begin
            counter <= 1; // tick zero is this one
            interrupt <= 1;
            wdata <= ~data[7];
        end
        else
        begin
            counter <= counter + 7'b1;
            interrupt <= 0;
            wdata <= 0;
        end
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
