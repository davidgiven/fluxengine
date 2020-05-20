
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"

//`#end` -- edit above this line, do not edit this line
// Generated on 12/11/2019 at 21:18
// Component: Sampler
module Sampler (
	output [2:0] debug_state,
	output reg [7:0] opcode,
	output reg       req,
	input   clock,
	input   index,
	input   rdata,
	input   reset,
	input   sampleclock
);

//`#start body` -- edit after this line, do not edit this line

reg [5:0] counter;

reg sampleclock_q;
reg index_q;
reg rdata_q;

reg sampleclock_edge;
reg index_edge;
reg rdata_edge;

always @(posedge clock)
begin
    if (reset)
    begin
        sampleclock_edge <= 0;
        index_edge <= 0;
        rdata_edge <= 0;
        sampleclock_q <= 0;
        index_q <= 0;
        rdata_q <= 0;
        counter <= 0;
    end
    else
    begin
        /* Remember positive egdes for sampleclock, index and rdata. */
        
        /* Positive-going edge detection of 16 MHz square-wave sample clock vs.
         * 64 MHz clock.
         */
        sampleclock_edge <= sampleclock && !sampleclock_q;
        sampleclock_q <= sampleclock;
        
        /* Request to write FIFO is inactive by default */
        req <= 0;
        
        if (sampleclock_edge) begin
            /* Both index and rdata are active high -- positive-going edges
             * indicate the start of an index pulse and read pulse, respectively.
             */
             
            index_edge <= index && !index_q;
            index_q <= index;
            
            rdata_edge <= rdata && !rdata_q;
            rdata_q <= rdata;
            
            if (rdata_edge || index_edge || (counter == 6'h3f)) begin
                opcode <= { rdata_edge, index_edge, counter };
                req <= 1;
                counter <= 1;
            end else begin
                counter <= counter + 1;
            end
        end
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
