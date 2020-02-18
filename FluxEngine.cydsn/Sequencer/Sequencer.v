
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
localparam STATE_WAITING = 1;
localparam STATE_PULSING = 2;
localparam STATE_INDEXING = 3;

localparam OPCODE_PULSE = 8'h80;
localparam OPCODE_INDEX = 8'h81;

reg [1:0] state;
reg [6:0] countdown;

assign req = (!reset && (state == STATE_LOAD));
assign wdata = (state == STATE_PULSING);
assign debug_state = state;

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
    end
    else
    begin
        if (!oldsampleclock && sampleclock)
            sampleclocked <= 1;
        oldsampleclock <= sampleclock;
        
        case (state)
            STATE_LOAD:
                /* Wait for a posedge on dataclocked, indicating an opcode has
                 * arrived. */
                if (dataclocked)
                    case (opcode)
                        OPCODE_PULSE:
                            state <= STATE_PULSING;
                        
                        OPCODE_INDEX:
                            state <= STATE_INDEXING;
                        
                        default:
                        begin
                            countdown <= opcode[6:0];
                            state <= STATE_WAITING;
                        end
                    endcase
            
            STATE_WAITING:
                if (sampleclocked)
                begin
                    sampleclocked <= 0;
                    countdown <= countdown - 1;
                    /* Nasty fudge factor here to account for one to two
                     * sample ticks lost per pulse. */
                    if (countdown <= 2)
                        state <= STATE_LOAD;
                end
            
            STATE_PULSING:
                state <= STATE_LOAD;
            
            STATE_INDEXING:
                if (indexed)
                    state <= STATE_LOAD;
        endcase
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
