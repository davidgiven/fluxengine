#include "cyfitter_cfg.h"
#include "cydevice_trm.h"
#include "cyfitter.h"
#include "`$INSTANCE_NAME`_h.h"

void `$INSTANCE_NAME`_Start()
{
   `$INSTANCE_NAME`_Init();
}    

void `$INSTANCE_NAME`_Stop()
{
    `$INSTANCE_NAME`_Disable();
}

void `$INSTANCE_NAME`_Init()
{    
    `$INSTANCE_NAME`_Enable();
    
}
void `$INSTANCE_NAME`_Enable()
{
}

void `$INSTANCE_NAME`_Disable()
{
}
