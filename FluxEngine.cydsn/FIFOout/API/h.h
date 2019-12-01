#if !defined(`$INSTANCE_NAME`_H)
#define `$INSTANCE_NAME`_H

#include "cytypes.h"
#include "cyfitter.h"
#include "CyLib.h" 

#define `$INSTANCE_NAME`_FIFO_PTR	         ((reg8 *) `$INSTANCE_NAME`_dp__F0_REG)

    /* Macros to clear DP FIFOs.*/
#define `$INSTANCE_NAME`_CLEAR do { \
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0x01u | \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)));\
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0xfeu & \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)));\
    } while(0)

/* Macros to set FIFO level mode. See the TRM for details */
#define `$INSTANCE_NAME`_SET_LEVEL_NORMAL \
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0xfbu & \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)))
#define `$INSTANCE_NAME`_SET_LEVEL_MID \
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0x04u | \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)))

/* Macros to set FIFO to single-buffer mode. */
#define `$INSTANCE_NAME`_SINGLE_BUFFER_SET \
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0x01u | \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)))

/* Macros to return the FIFO to normal mode. */
#define `$INSTANCE_NAME`_SINGLE_BUFFER_UNSET \
    CY_SET_XTND_REG8(\
        ((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG), 0xfeu & \
        CY_GET_XTND_REG8(((reg8 *) `$INSTANCE_NAME`_dp__DP_AUX_CTL_REG)))
    
void `$INSTANCE_NAME`_Enable();
void `$INSTANCE_NAME`_Disable();
void `$INSTANCE_NAME`_Start();
void `$INSTANCE_NAME`_Stop();
void `$INSTANCE_NAME`_Init();

#endif

/* [] END OF FILE */
