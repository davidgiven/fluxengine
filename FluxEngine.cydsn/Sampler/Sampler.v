
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"

//`#end` -- edit above this line, do not edit this line
// Generated on 12/11/2019 at 21:18
// Component: Sampler
module Sampler (
	output [2:0] debug_state,
	output reg [7:0] opcode,
	output  reg       req,
	input   clock,
	input   index,
	input   rdata,
	input   reset,
	input   sampleclock
);

//`#start body` -- edit after this line, do not edit this line

// NOTE: Reset pulse is used in both clock domains, and must be long enough
// to be detected in both.

reg [5:0] counter;

reg index_edge;
reg rdata_edge;

reg req_toggle;

reg rdata_toggle;
reg old_rdata_toggle;

reg index_toggle;
reg old_index_toggle;

always @(posedge rdata)
begin
    rdata_toggle <= ~rdata_toggle;
end

always @(posedge index)
begin
    index_toggle <= ~index_toggle;
end

always @(posedge sampleclock)
begin
    if (reset)
    begin
        old_rdata_toggle <= 0;
        old_index_toggle <= 0;
        
        index_edge <= 0;
        rdata_edge <= 0;
        counter <= 0;
        req_toggle <= 0;
    end
    else
    begin
        /* If data_toggle or index_toggle have changed state, this means that they've
         * gone high since the last sampleclock. */
         
        index_edge <= index_toggle != old_index_toggle;
        old_index_toggle <= index_toggle;
        
        rdata_edge <= rdata_toggle != old_rdata_toggle;
        old_rdata_toggle <= rdata_toggle;
        
        if (rdata_edge || index_edge || (counter == 6'h3f)) begin
            opcode <= { rdata_edge, index_edge, counter };
            req_toggle <= ~req_toggle;
            counter <= 1;   /* remember to count this tick */
        end else begin
            counter <= counter + 1;
        end
    end
end

reg req_toggle_q;

always @(posedge clock)
begin
    if (reset) begin
        req_toggle_q <= 0;
        req <= 0;
    end else begin
        req_toggle_q <= req_toggle;
        req <= (req_toggle != req_toggle_q);
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
