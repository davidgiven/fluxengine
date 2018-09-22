/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

int main(void)
{
    CyGlobalIntEnable;
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    USBFS_CDC_Init();

    for(;;)
    {
        if (!USBFS_GetConfiguration() || USBFS_IsConfigurationChanged())
        {
            while (!USBFS_GetConfiguration())
                ;
        }

        USBFS_PutString("Hello, world!\r\n");
        CyDelay(1000);
    }
}
