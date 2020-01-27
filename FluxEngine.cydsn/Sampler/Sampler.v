
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

localparam STATE_RESET = 0;
localparam STATE_WAITING = 1;
localparam STATE_INTERVAL = 2;
localparam STATE_DISPATCH = 3;
localparam STATE_OPCODE = 4;
localparam STATE_COUNTING = 5;

reg [2:0] state;
wire [6:0] counter;

wire countnow;
assign countnow = (state == STATE_COUNTING);

wire counterreset;
assign counterreset = (state == STATE_INTERVAL) || (state == STATE_OPCODE);

SuperCounter #(.Delta(1), .ResetValue(0)) Counter
(
    /* input */ .clk(clock),
    /* input */ .reset(counterreset),
    /* input */ .count(countnow),
    /* output */ .d(counter)
);

reg oldsampleclock;
wire sampleclocked;
assign sampleclocked = !oldsampleclock && sampleclock;

reg oldindex;
wire indexed;
assign indexed = !oldindex && index;

wire rdataed;
reg oldrdata;
assign rdataed = !oldrdata && rdata;

assign req = (state == STATE_INTERVAL) || (state == STATE_OPCODE);

always @(posedge clock)
begin
    if (reset)
    begin
        state <= STATE_RESET;
        opcode <= 0;
        oldsampleclock <= 0;
        oldindex <= 0;
        oldrdata <= 0;
    end
    else
        case (state)
            STATE_RESET:
                state <= STATE_WAITING;
            
            STATE_WAITING:
            begin
                if (rdataed || indexed)
                begin
                    opcode <= {0, counter};
                    state <= STATE_INTERVAL;
                end
                else if (sampleclocked)
                begin
                    oldsampleclock <= 1;
                    if (counter == 7'h7f)
                    begin
                        opcode <= {0, counter};
                        state <= STATE_OPCODE;
                    end
                    else
                        state <= STATE_COUNTING;
                end
                
                if (oldrdata && !rdata)
                    oldrdata <= 0;
                if (oldindex && !index)
                    oldindex <= 0;
                if (oldsampleclock && !sampleclock)
                    oldsampleclock <= 0;
            end
            
            STATE_INTERVAL: /* interval byte sent here; counter reset */
                state <= STATE_DISPATCH;
                
            STATE_DISPATCH: /* relax after interval byte, dispatch for opcode */
            begin
                if (rdataed)
                begin
                    oldrdata <= 1;
                    opcode <= 8'h80;
                    state <= STATE_OPCODE;
                end
                else if (indexed)
                begin
                    oldindex <= 1;
                    opcode <= 8'h81;
                    state <= STATE_OPCODE;
                end
                else
                    state <= STATE_WAITING;
            end
            
            STATE_OPCODE: /* opcode byte sent here */
                state <= STATE_WAITING;
                            
            STATE_COUNTING: /* counter changes here */
                state <= STATE_WAITING;
        endcase
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
