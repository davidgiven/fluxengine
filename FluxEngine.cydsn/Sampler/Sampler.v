
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
`include "../SuperCounter/SuperCounter.v"

//`#end` -- edit above this line, do not edit this line
// Generated on 12/11/2019 at 21:18
// Component: Sampler
module Sampler (
	output [2:0] debug_state,
	output reg [7:0] opcode,
	output  req,
	input   clock,
	input   index,
	input   rdata,
	input   reset,
	input   sampleclock
);

//`#start body` -- edit after this line, do not edit this line

localparam STATE_WAITING = 0;
localparam STATE_OPCODE = 1;

reg [0:0] state;
reg [5:0] counter;

reg oldsampleclock;
reg oldindex;
reg oldrdata;

reg sampleclocked;
reg indexed;
reg rdataed;

assign req = (state == STATE_OPCODE);

always @(posedge clock)
begin
    if (reset)
    begin
        state <= STATE_WAITING;
        opcode <= 0;
        sampleclocked <= 0;
        indexed <= 0;
        rdataed <= 0;
        oldsampleclock <= 0;
        oldindex <= 0;
        oldrdata <= 0;
        counter <= 0;
    end
    else
    begin
        /* Remember positive egdes for sampleclock, index and rdata. */
        
        if (sampleclock && !oldsampleclock)
            sampleclocked <= 1;
        oldsampleclock <= sampleclock;
        
        if (index && !oldindex)
            indexed <= 1;
        oldindex <= index;
        
        if (rdata && !oldrdata)
            rdataed <= 1;
        oldrdata <= rdata;
        
        case (state)
            STATE_WAITING:
            begin
                if (sampleclocked)
                begin
                    if (rdataed || indexed || (counter == 6'h3f))
                    begin
                        opcode <= {rdataed, indexed, counter};
                        rdataed <= 0;
                        indexed <= 0;
                        counter <= 1; /* remember to count this tick */
                        state <= STATE_OPCODE;
                    end
                    else
                        counter <= counter + 1;
                        
                    sampleclocked <= 0;
                end
            end
            
            STATE_OPCODE: /* opcode sent here */
            begin
                state <= STATE_WAITING;
            end
        endcase
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
