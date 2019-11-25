
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

localparam STATE_IDLE = 0;
localparam STATE_LOAD = 1;
localparam STATE_WAITING = 2;
localparam STATE_PULSING = 3;
localparam STATE_INDEXING = 4;

localparam OPCODE_PULSE = 8'h80;
localparam OPCODE_INDEX = 8'h81;

reg [2:0] state;
reg [6:0] countdown;

assign req = (state == STATE_LOAD);
assign wdata = (state == STATE_PULSING);
assign debug_state = state;

reg olddataclock;
wire dataclocked;
always @(posedge clock) olddataclock <= dataclock;
assign dataclocked = !olddataclock && dataclock;

reg oldsampleclock;
wire sampleclocked;
always @(posedge clock) oldsampleclock <= sampleclock;
assign sampleclocked = !oldsampleclock && sampleclock;

reg oldindex;
wire indexed;
always @(posedge clock) oldindex <= index;
assign indexed = !oldindex && index;

always @(posedge clock)
begin
    if (reset)
    begin
        state <= STATE_IDLE;
        countdown <= 0;
    end
    else
    begin
        case (state)
            STATE_IDLE:
            begin
                state <= STATE_LOAD;
            end
            
            STATE_LOAD:
            begin
                if (dataclocked)
                begin
                    case (opcode)
                        OPCODE_PULSE:
                        begin
                            state <= STATE_PULSING;
                        end
                        
                        OPCODE_INDEX:
                        begin
                            state <= STATE_INDEXING;
                        end
                        
                        default:
                        begin
                            countdown <= opcode[6:0];
                            state <= STATE_WAITING;
                        end
                    endcase
                end
            end
            
            STATE_WAITING:
            begin
                if (sampleclocked)
                begin
                    if (countdown == 0)
                    begin
                        state <= STATE_LOAD;
                    end
                    else
                    begin
                        countdown <= countdown - 1;
                    end
                end
            end
            
            STATE_PULSING:
            begin
                state <= STATE_LOAD;
            end
            
            STATE_INDEXING:
            begin
                if (indexed)
                begin
                    state <= STATE_LOAD;
                end
            end
        endcase
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
