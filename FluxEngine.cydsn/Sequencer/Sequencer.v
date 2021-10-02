
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 11/24/2019 at 17:25
// Component: Sequencer
module Sequencer (
	output req, /* request new data on leading edge */
	output wdata,
    output [2:0] debug_state,
	input clock,
	input dataclock, /* incoming data on leading edge */
	input [7:0] opcode,
    input index,
    input sampleclock,
    input reset
);

//`#start body` -- edit after this line, do not edit this line

localparam STATE_LOAD = 0;
localparam STATE_WRITING = 1;

reg state;
reg [5:0] countdown;
reg pulsepending;

assign req = (!reset && (state == STATE_LOAD));
assign wdata = (!reset && (state == STATE_WRITING) && (countdown == 0) && pulsepending);
assign debug_state = 0;

reg olddataclock;
wire dataclocked;
always @(posedge clock) olddataclock <= dataclock;
assign dataclocked = !olddataclock && dataclock;

reg oldsampleclock;
reg sampleclocked;

reg oldindex;
wire indexed;
always @(posedge clock) oldindex <= index;
assign indexed = !oldindex && index;

always @(posedge clock)
begin
    if (reset)
    begin
        state <= STATE_LOAD;
        countdown <= 0;
        pulsepending <= 0;
        oldsampleclock <= 0;
    end
    else
    begin
        if (!oldsampleclock && sampleclock)
            sampleclocked <= 1;
        oldsampleclock <= sampleclock;
            
        case (state)
            STATE_LOAD:
            begin
                /* A posedge on dataclocked indicates that another opcode has
                 * arrived. */
                if (dataclocked)
                begin
                    pulsepending <= opcode[7];
                    if (opcode[5:0] == 0)
                        countdown <= 0;
                    else
                        countdown <= opcode[5:0] - 1; /* compensate for extra tick in state machine */
                    
                    state <= STATE_WRITING;
                end
            end
            
            STATE_WRITING:
            begin
                if (sampleclocked)
                begin
                    if (countdown == 0)
                        state <= STATE_LOAD;
                    else
                        countdown <= countdown - 1;
                    sampleclocked <= 0;
                end
            end
        endcase
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
