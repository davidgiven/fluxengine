#include <libusbp_internal.h>
#include <mach/mach_time.h>

struct libusbp_generic_handle
{
    // ioh is short for "IOKit Handle"
    IOUSBInterfaceInterface182 ** ioh;

    IOCFPlugInInterface ** plug_in;

    mach_port_t async_port;

    // Timeouts are stored in milliseconds.  0 is forever.
    uint32_t out_timeout[MAX_ENDPOINT_NUMBER + 1];
    uint32_t in_timeout[MAX_ENDPOINT_NUMBER + 1];

    // These arrays allow us to convert from a normal USB endpoint address like
    // 0x82 (Endpoint 2 IN) to the pipe index needed by IOUSBInterface functions.
    uint8_t out_pipe_index[MAX_ENDPOINT_NUMBER + 1];
    uint8_t in_pipe_index[MAX_ENDPOINT_NUMBER + 1];
};

#ifdef LIBUSBP_LOG
void log_mach_msg(mach_msg_return_t mr, mach_msg_header_t * header)
{
    uint64_t t = mach_absolute_time();
    size_t main_payload_size = header->msgh_size - sizeof(mach_msg_header_t);
    assert(main_payload_size < 1024);  // avoid buffer overruns
    uint8_t * payload = (uint8_t *)(header + 1);
    mach_msg_trailer_t * trailer = (mach_msg_trailer_t *)(payload + main_payload_size);
    size_t payload_size = main_payload_size + trailer->msgh_trailer_size;
    assert(payload_size < 1024);  // avoid buffer overruns

    printf("mach_msg at %lld: %#x, %s, %d\n", t, mr, mach_error_string(mr), header->msgh_size);
    printf("  bits: %#x\n", header->msgh_bits);
    printf("  ports: %d, %d, %d\n", header->msgh_remote_port,
        header->msgh_local_port, header->msgh_voucher_port);
    printf("  id: %d\n", header->msgh_id);
    for (size_t i = 0; i < payload_size; i++)
    {
        if ((i % 8) == 0) { printf("    "); }
        printf("%02x ", payload[i]);
        if ((i % 8) == 7 || i == payload_size - 1) { printf("\n"); }
    }
}
#endif

// Gets the properties of all the pipes and uses that to populate the
// out_pipe_index and in_pipe_index arrays.
static libusbp_error * process_pipe_properties(libusbp_generic_handle * handle)
{
    uint8_t endpoint_count;
    kern_return_t kr = (*handle->ioh)->GetNumEndpoints(handle->ioh, &endpoint_count);
    if (kr != KERN_SUCCESS)
    {
        return error_create_mach(kr, "Failed to get number of endpoints.");
    }

    for(uint32_t i = 1; i <= endpoint_count; i++)
    {
        uint8_t direction;
        uint8_t endpoint_number;
        uint8_t transfer_type;
        uint16_t max_packet_size;
        uint8_t interval;
        kern_return_t kr = (*handle->ioh)->GetPipeProperties(handle->ioh, i,
            &direction, &endpoint_number, &transfer_type, &max_packet_size, &interval);
        if (kr != KERN_SUCCESS)
        {
            return error_create_mach(kr, "Failed to get pipe properties for pipe %d.", i);
        }

        if (endpoint_number <= MAX_ENDPOINT_NUMBER)
        {
            if (direction)
            {
                handle->in_pipe_index[endpoint_number] = i;
            }
            else
            {
                handle->out_pipe_index[endpoint_number] = i;
            }
        }
    }
    return NULL;
}

// Sets the configruation of a device to 1 if it is not configured.
static libusbp_error * set_configuration(io_service_t service)
{
    assert(service != MACH_PORT_NULL);

    libusbp_error * error = NULL;

    // Turn io_service_t into something we can actually use.
    IOUSBDeviceInterface ** dev_handle = NULL;
    IOCFPlugInInterface ** plug_in = NULL;
    if (error == NULL)
    {
        error = service_to_interface(service,
            kIOUSBDeviceUserClientTypeID,
            CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197),
            (void **)&dev_handle,
            &plug_in);
    }

    uint8_t config_num = 0;
    if (error == NULL)
    {
        kern_return_t kr = (*dev_handle)->GetConfiguration(dev_handle, &config_num);
        if (kr != KERN_SUCCESS)
        {
            // We failed to get the current configuration.  The documentation of
            // GetConfiguration doesn't state whether it actually does I/O on
            // the device or not, but if it does do I/O then one possible reason
            // for GetConfiguration to fail is that the device simply doesn't
            // support the request.  Let's just assume the device is not
            // configured and set it to configuration 1.
            config_num = 0;
        }
    }

    // Open the device for exclusive access.
    if (error == NULL && config_num == 0)
    {
        kern_return_t kr = (*dev_handle)->USBDeviceOpen(dev_handle);
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "Failed to open handle to device.");
        }
    }

    // Set the configuration.
    if (error == NULL && config_num == 0)
    {
        uint8_t new_config_num = 1;
        kern_return_t kr = (*dev_handle)->SetConfiguration(dev_handle, new_config_num);
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "Failed to set configuration to 1.");
        }
    }

    // Clean up.
    if (dev_handle != NULL)
    {
        (*dev_handle)->USBDeviceClose(dev_handle);
        (*dev_handle)->Release(dev_handle);
        dev_handle = NULL;
        (*plug_in)->Release(plug_in);
        plug_in = NULL;
    }

    return error;
}

// Sets the configruation of the device to 1 if it is not set, and then
// retrieves the io_service_t representing the specific interface we want
// to talk to.
static libusbp_error * set_configuration_and_get_service(
    const libusbp_generic_interface * gi,
    io_service_t * service)
{
    assert(gi != NULL);
    assert(service != NULL);
    *service = MACH_PORT_NULL;

    uint64_t device_id = generic_interface_get_device_id(gi);
    uint8_t interface_number = generic_interface_get_interface_number(gi);

    libusbp_error * error = NULL;

    // Get an io_service_t for the physical device.
    io_service_t device_service = MACH_PORT_NULL;
    if (error == NULL)
    {
        error = service_get_from_id(device_id, &device_service);
    }

    // Set the configruation to 1 if it is not set.
    if (error == NULL)
    {
        error = set_configuration(device_service);
    }

    // Get the io_service_t for the interface.
    if (error == NULL)
    {
        error = service_get_usb_interface(device_service, interface_number, service);
    }

    if (device_service != MACH_PORT_NULL) { IOObjectRelease(device_service); }

    return error;
}

libusbp_error * libusbp_generic_handle_open(
    const libusbp_generic_interface * gi,
    libusbp_generic_handle ** handle)
{
    if (handle == NULL)
    {
        return error_create("Generic handle output pointer is null.");
    }

    *handle = NULL;

    if (gi == NULL)
    {
        return error_create("Generic interface is null.");
    }

    libusbp_error * error = NULL;

    // Allocate memory for the handle.
    libusbp_generic_handle * new_handle = NULL;
    if (error == NULL)
    {
        new_handle = calloc(1, sizeof(libusbp_generic_handle));
        if (new_handle == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Get the io_service_t representing the IOUSBInterface.
    io_service_t service = MACH_PORT_NULL;
    if (error == NULL)
    {
        uint64_t interface_id;
        bool has_interface_id = generic_interface_get_interface_id(gi, &interface_id);
        if (has_interface_id)
        {
            // This generic interface has an I/O Registry ID for a specific USB interface,
            // so lets just get the corresponding io_service_t.
            error = service_get_from_id(interface_id, &service);
        }
        else
        {
            // This generic interface does not have an ID for the specific USB interface yet,
            // probably because it is a non-composite device and we need to put it into the
            // right configuration.

            error = set_configuration_and_get_service(gi, &service);
        }
    }

    // Get the IOInterfaceInterface
    if (error == NULL)
    {
        error = service_to_interface(service,
            kIOUSBInterfaceUserClientTypeID,
            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID182),
            (void **)&new_handle->ioh,
            &new_handle->plug_in);
    }

    // Open the interface for exclusive access.
    // (Otherwise, we can't read from non-zero pipes.)
    if (error == NULL)
    {
        kern_return_t kr = (*new_handle->ioh)->USBInterfaceOpen(new_handle->ioh);
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "Failed to open generic handle.");
        }
    }

    // Retrieve and store important information from the pipe properties.
    if (error == NULL)
    {
        error = process_pipe_properties(new_handle);
    }

    // Create the async port.
    if (error == NULL)
    {
        kern_return_t kr = (*new_handle->ioh)->CreateInterfaceAsyncPort(new_handle->ioh,
            &new_handle->async_port);
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "Failed to create asynchronous port.");
        }
    }

    // Pass the handle to the caller.
    if (error == NULL)
    {
        *handle = new_handle;
        new_handle = NULL;
    }

    // Clean up.
    libusbp_generic_handle_close(new_handle);
    if (service != MACH_PORT_NULL) { IOObjectRelease(service); }
    return error;
}

void libusbp_generic_handle_close(libusbp_generic_handle * handle)
{
    if (handle != NULL)
    {
        if (handle->ioh != NULL)
        {
            (*handle->ioh)->USBInterfaceClose(handle->ioh);
            (*handle->ioh)->Release(handle->ioh);
            (*handle->plug_in)->Release(handle->plug_in);
        }
        free(handle);
    }
}

libusbp_error * libusbp_generic_handle_open_async_in_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    libusbp_async_in_pipe ** pipe)
{
    return async_in_pipe_create(handle, pipe_id, pipe);
}

libusbp_error * libusbp_generic_handle_set_timeout(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    uint32_t timeout)
{
    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL)
    {
        error = check_pipe_id(pipe_id);
    }

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;

        if (pipe_id & 0x80)
        {
            handle->in_timeout[endpoint_number] = timeout;
        }
        else
        {
            handle->out_timeout[endpoint_number] = timeout;
        }
    }

    return error;
}

libusbp_error * libusbp_control_transfer(
    libusbp_generic_handle * handle,
    uint8_t bmRequestType,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    void * buffer,
    uint16_t wLength,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    IOUSBDevRequestTO request;
    request.bmRequestType = bmRequestType;
    request.bRequest = bRequest;
    request.wValue = wValue;
    request.wIndex = wIndex;
    request.wLength = wLength;
    request.pData = buffer;
    request.completionTimeout = handle->out_timeout[0];
    request.wLenDone = 0;

    kern_return_t kr = (*handle->ioh)->ControlRequestTO(handle->ioh, 0, &request);
    if (transferred != NULL) { *transferred = request.wLenDone; }
    if (kr != KERN_SUCCESS)
    {
        return error_create_mach(kr, "Control transfer failed.");
    }

    return NULL;
}

libusbp_error * libusbp_read_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    void * buffer,
    size_t size,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL && size == 0)
    {
        error = error_create("Transfer size 0 is not allowed.");
    }

    if (error == NULL && size > UINT32_MAX)
    {
        error = error_create("Transfer size is too large.");
    }

    if (error == NULL && buffer == NULL)
    {
        error = error_create("Buffer is null.");
    }

    if (error == NULL)
    {
        error = check_pipe_id_in(pipe_id);
    }

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;
        uint32_t no_data_timeout = 0;
        uint32_t completion_timeout = handle->in_timeout[endpoint_number];
        uint32_t iokit_size = size;
        uint32_t pipe_index = handle->in_pipe_index[endpoint_number];
        kern_return_t kr = (*handle->ioh)->ReadPipeTO(handle->ioh, pipe_index,
          buffer, &iokit_size, no_data_timeout, completion_timeout);
        if (transferred != NULL) { *transferred = iokit_size; }
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "");
        }
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to read from pipe.");
    }

    return error;
}

libusbp_error * libusbp_write_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    const void * buffer,
    size_t size,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL && size > UINT32_MAX)
    {
        error = error_create("Transfer size is too large.");
    }

    if (error == NULL && buffer == NULL && size)
    {
        error = error_create("Buffer is null.");
    }

    if (error == NULL)
    {
        error = check_pipe_id_out(pipe_id);
    }

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;
        uint32_t no_data_timeout = 0;
        uint32_t completion_timeout = handle->out_timeout[endpoint_number];
        uint32_t pipe_index = handle->out_pipe_index[endpoint_number];
        kern_return_t kr = (*handle->ioh)->WritePipeTO(handle->ioh, pipe_index,
          (void *)buffer, size, no_data_timeout, completion_timeout);
        if (kr != KERN_SUCCESS)
        {
            error = error_create_mach(kr, "");
        }
    }

    if (error == NULL && transferred != NULL)
    {
        // There was no error, so just assume the entire amount was transferred.
        // WritePipeTO does not give us a number.
        *transferred = size;
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to write to pipe.");
    }

    return error;
}

#pragma pack(4)
typedef struct
{
    mach_msg_header_t header;
    uint8_t buffer[1024];
} our_msg;

libusbp_error * generic_handle_events(libusbp_generic_handle * handle)
{
    assert(handle != NULL);

    while(1)
    {
        // Check for messages on this handle's async port using mach_msg().
        our_msg msg;
        mach_msg_option_t option = MACH_RCV_MSG | MACH_RCV_TIMEOUT;
        option |= MACH_RCV_LARGE;  // tmphax to receive large messages
        mach_msg_size_t send_size = 0;
        mach_msg_size_t rcv_size = sizeof(msg); // might need to make this bigger
        mach_msg_timeout_t timeout = 0;  // non-blocking
        mach_port_t notify = MACH_PORT_NULL;
        mach_msg_return_t mr = mach_msg(&msg.header, option, send_size, rcv_size,
            handle->async_port, timeout, notify);

        if (mr != MACH_MSG_SUCCESS)
        {
            if (mr == MACH_RCV_TIMED_OUT)
            {
                // There was no message available on the port.
                return NULL;
            }
            else if (mr == MACH_RCV_TOO_LARGE)
            {
                return error_create("Mach message received was too large: %d > %d\n",
                    msg.header.msgh_size, (unsigned int)sizeof(msg));
            }
            else
            {
                return error_create_mach(mr, "Failed to receive mach message.");
            }
        }

        #ifdef LIBUSBP_LOG
        log_mach_msg(mr, &msg.header);
        #endif

        // We have received a message from the mach port that contains a blob of
        // binary data.  The blob seems to include a pointer to the
        // async_in_transfer, a kern_return_t code for the transfer, and a pointer
        // to the callback we specified when we submitted the transfer.  We aren't
        // supposed to process that data ourselves; we are supposed to call
        // IODispatchCalloutFromMessage.
        //
        // The third argument to IODispatchCalloutFromMessage is supposed to be a
        // IONotificationPortRef, but we don't have access to the
        // IONotificationPortRef, which is a protected member of IOUSBInterface
        // class with no accessors.  Passing NULL (or any arbitary pointer) seems to
        // work.  IODispatchCalloutFromMessage has no return value.
        IODispatchCalloutFromMessage(0, &msg.header, NULL);
    }
}

void ** libusbp_generic_handle_get_cf_plug_in(libusbp_generic_handle * handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    return (void **)handle->plug_in;
}

IOUSBInterfaceInterface182 ** generic_handle_get_ioh(const libusbp_generic_handle * handle)
{
    assert(handle != NULL);
    return handle->ioh;
}

uint8_t generic_handle_get_pipe_index(const libusbp_generic_handle * handle, uint8_t pipe_id)
{
    uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;
    if (pipe_id & 0x80)
    {
        return handle->in_pipe_index[endpoint_number];
    }
    else
    {
        return handle->out_pipe_index[endpoint_number];
    }
}
