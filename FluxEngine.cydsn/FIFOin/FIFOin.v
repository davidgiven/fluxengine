
//`#start header` -- edit after this line, do not edit this line
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line

/* Ultra-simple FIFO in component: a byte is shifted in every clock when req
 * is high. */
 
module FIFOin (drq, clk, d, req);
	output  drq;
	input   clk;
	input  [7:0] d;
	input  req;

//`#start body` -- edit after this line, do not edit this line

wire [7:0] pi;
assign pi = d;

wire load;
assign load = req;

cy_psoc3_dp #(.cy_dpconfig(
{
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM0:    */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM1:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM2:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM3:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM4:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM5:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM6:     */
	`CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
	`CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
	`CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
	`CS_CMP_SEL_CFGA, /*CFGRAM7:     */
	8'hFF, 8'h00,	/*CFG9:     */
	8'hFF, 8'hFF,	/*CFG11-10:     */
	`SC_CMPB_A1_D1, `SC_CMPA_A1_D1, `SC_CI_B_ARITH,
	`SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
	`SC_A_MASK_DSBL, `SC_DEF_SI_0, `SC_SI_B_DEFSI,
	`SC_SI_A_DEFSI, /*CFG13-12:     */
	`SC_A0_SRC_PIN, `SC_SHIFT_SL, 1'h0,
	1'h0, `SC_FIFO1_BUS, `SC_FIFO0_ALU,
	`SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
	`SC_FB_NOCHN, `SC_CMP1_NOCHN,
	`SC_CMP0_NOCHN, /*CFG15-14:     */
	10'h00, `SC_FIFO_CLK__DP,`SC_FIFO_CAP_AX,
	`SC_FIFO_LEVEL,`SC_FIFO__SYNC,`SC_EXTCRC_DSBL,
	`SC_WRK16CAT_DSBL /*CFG17-16:     */
}
)) dp(
	/* input          */ .clk(clk),
	/* input [02:00]  */ .cs_addr(3'b0),    // Program counter
	/* input          */ .route_si(1'b0),   // Shift in
	/* input          */ .route_ci(1'b0),   // Carry in
	/* input          */ .f0_load(load),    // Load FIFO 0
	/* input          */ .f1_load(1'b0), 	// Load FIFO 1
	/* input          */ .d0_load(1'b0), 	// Load Data Register 0
	/* input          */ .d1_load(1'b0), 	// Load Data Register 1
	/* output         */ .ce0(), 			// Accumulator 0 = Data register 0
	/* output         */ .cl0(), 			// Accumulator 0 < Data register 0
	/* output         */ .z0(), 			// Accumulator 0 = 0
	/* output         */ .ff0(), 			// Accumulator 0 = FF
	/* output         */ .ce1(), 			// Accumulator [0|1] = Data register 1
	/* output         */ .cl1(), 			// Accumulator [0|1] < Data register 1
	/* output         */ .z1(), 			// Accumulator 1 = 0
	/* output         */ .ff1(), 			// Accumulator 1 = FF
	/* output         */ .ov_msb(), 		// Operation over flow
	/* output         */ .co_msb(), 		// Carry out
	/* output         */ .cmsb(), 			// Carry out
	/* output         */ .so(), 			// Shift out
    /* output         */ .f0_bus_stat(drq), // not empty
	/* output         */ .f0_blk_stat(full),// full
	/* output         */ .f1_bus_stat(), 	// FIFO 1 status to uP
	/* output         */ .f1_blk_stat(), 	// FIFO 1 status to DP
	/* input          */ .ci(1'b0), 		// Carry in from previous stage
	/* output         */ .co(), 			// Carry out to next stage
	/* input          */ .sir(1'b0), 		// Shift in from right side
	/* output         */ .sor(), 			// Shift out to right side
	/* input          */ .sil(1'b0), 		// Shift in from left side
	/* output         */ .sol(), 			// Shift out to left side
	/* input          */ .msbi(1'b0), 		// MSB chain in
	/* output         */ .msbo(), 			// MSB chain out
	/* input [01:00]  */ .cei(2'b0),        // Compare equal in from prev stage
	/* output [01:00] */ .ceo(),            // Compare equal out to next stage
	/* input [01:00]  */ .cli(2'b0), 	    // Compare less than in from prv stage
	/* output [01:00] */ .clo(),            // Compare less than out to next stage
	/* input [01:00]  */ .zi(2'b0),         // Zero detect in from previous stage
	/* output [01:00] */ .zo(),             // Zero detect out to next stage
	/* input [01:00]  */ .fi(2'b0), 		// 0xFF detect in from previous stage
	/* output [01:00] */ .fo(), 	        // 0xFF detect out to next stage
	/* input [01:00]  */ .capi(2'b0),	    // Capture in from previous stage
	/* output [01:00] */ .capo(),		    // Capture out to next stage
	/* input          */ .cfbi(1'b0), 		// CRC Feedback in from previous stage
	/* output         */ .cfbo(), 			// CRC Feedback out to next stage
	/* input [07:00]  */ .pi(pi), 		    // Parallel data port
	/* output [07:00] */ .po()              // Parallel data port
);

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line




