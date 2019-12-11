
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
localparam STATE_OPCODE = 2;

reg [1:0] state;
wire [6:0] counter;

SuperCounter #(.Delta(1), .ResetValue(0)) Counter
(
    /* input */ .clk(clock),
    /* input */ .reset(counterreset),
    /* input */ .count(countnow),
    /* output */ .d(counter)
);

wire countnow;
wire counterreset;

reg oldsampleclock;
reg oldindex;
reg oldrdata;

assign req = (state == STATE_OPCODE);

always @(posedge clock)
begin
    if (reset)
    begin
        state <= STATE_RESET;
        opcode <= 0;
        oldsampleclock <= 0;
        oldindex <= 0;
        oldrdata <= 0;
        counterreset <= 1;
        countnow <= 0;
    end
    else
        case (state)
            STATE_RESET:
            begin
                state <= STATE_WAITING;
                countnow <= 0;
                counterreset <= 0;
            end
            
            STATE_WAITING:
            begin
                countnow <= 0;
                counterreset <= 0;
                
                if (rdata && !oldrdata)
                begin
                    oldrdata <= 1;
                    opcode <= 8'h80;
                    state <= STATE_OPCODE;
                end
                else if (index && !oldindex)
                begin
                    oldindex <= 1;
                    opcode <= 8'h81;
                    state <= STATE_OPCODE;
                end
                else if (sampleclock && !oldsampleclock)
                begin
                    oldsampleclock <= 1;
                    opcode <= {0, counter};
                    if (counter == 7'h7f)
                        state <= STATE_OPCODE;
                    else
                        countnow <= 1;
                end
                
                if (oldrdata && !rdata)
                    oldrdata <= 0;
                if (oldindex && !index)
                    oldindex <= 0;
                if (oldsampleclock && !sampleclock)
                    oldsampleclock <= 0;
            end
            
            STATE_OPCODE:
            begin
                counterreset <= 1;
                state <= STATE_WAITING;
            end
        endcase
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
