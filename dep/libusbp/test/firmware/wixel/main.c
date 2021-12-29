// This is test firmware that runs on a Wixel and can be used for
// testing generic USB PC software.
//
// By default, it compiles USB Test Device A, a composite device with two
// vendor-defined USB interfaces and a serial port.
//
// When compiled with the TEST_DEVICE_B option, it compiles USB Test Device B,
// which is a non-composite device with a single vendor-defined USB interface.

#include <board.h>
#include <usb.h>
#include <gpio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <adc.h>
#include "cdc_acm_constants.h"

#ifdef TEST_DEVICE_B
#define TEST_DEVICE_LETTER 'B'
#else
#define TEST_DEVICE_LETTER 'A'
#define COMPOSITE
#endif

//#define USE_MS_OS_10
#define USE_MS_OS_20

#define NATIVE_INTERFACE_0           0
#define ADC_DATA_ENDPOINT            2
#define ADC_DATA_FIFO                USBF2
#define ADC_DATA_PACKET_SIZE         5
#define CMD_ENDPOINT                 3
#define CMD_PACKET_SIZE              32
#define CMD_FIFO                     USBF3

#ifdef COMPOSITE
#define NATIVE_INTERFACE_1           1

#define CDC_OUT_PACKET_SIZE          64
#define CDC_IN_PACKET_SIZE           64
#define CDC_CONTROL_INTERFACE        2
#define CDC_DATA_INTERFACE           3
#define CDC_NOTIFICATION_ENDPOINT    1
#define CDC_NOTIFICATION_FIFO        USBF1
#define CDC_NOTIFICATION_PACKET_SIZE 10
#define CDC_DATA_ENDPOINT            4
#define CDC_DATA_FIFO                USBF4
#endif

#define REQUEST_GET_MS_DESCRIPTOR    0x20

// Wireless USB Specification 1.1, Table 7-1
#define USB_DESCRIPTOR_TYPE_SECURITY 12
#define USB_DESCRIPTOR_TYPE_KEY 13
#define USB_DESCRIPTOR_TYPE_ENCRYPTION_TYPE 14
#define USB_DESCRIPTOR_TYPE_BOS 15
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY 16
#define USB_DESCRIPTOR_TYPE_WIRELESS_ENDPOINT_COMPANION 17

// Microsoft OS 2.0 Descriptors, Table 1
#define USB_DEVICE_CAPABILITY_TYPE_PLATFORM 5

// Microsoft OS 2.0 Descriptors, Table 8
#define MS_OS_20_DESCRIPTOR_INDEX 7
#define MS_OS_20_SET_ALT_ENUMERATION 8

// Microsoft OS 2.0 Descriptors, Table 9
#define MS_OS_20_SET_HEADER_DESCRIPTOR 0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION 0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION 0x02
#define MS_OS_20_FEATURE_COMPATIBLE_ID 0x03
#define MS_OS_20_FEATURE_REG_PROPERTY 0x04
#define MS_OS_20_FEATURE_MIN_RESUME_TIME 0x05
#define MS_OS_20_FEATURE_MODEL_ID 0x06
#define MS_OS_20_FEATURE_CCGP_DEVICE 0x07

uint8_t CODE usbStringDescriptorCount = 7;
DEFINE_STRING_DESCRIPTOR(languagesString, 1, USB_LANGUAGE_EN_US)
DEFINE_STRING_DESCRIPTOR(manufacturerString, 18, 'P','o','l','o','l','u',' ','C','o','r','p','o','r','a','t','i','o','n')
DEFINE_STRING_DESCRIPTOR(productString, 17, 'U','S','B',' ','T','e','s','t',' ','D','e','v','i','c','e',' ',TEST_DEVICE_LETTER)
DEFINE_STRING_DESCRIPTOR(interface0String, 29, 'U','S','B',' ','T','e','s','t',' ','D','e','v','i','c','e',' ',TEST_DEVICE_LETTER,' ','I','n','t','e','r','f','a','c','e',' ','0')
DEFINE_STRING_DESCRIPTOR(interface1String, 29, 'U','S','B',' ','T','e','s','t',' ','D','e','v','i','c','e',' ',TEST_DEVICE_LETTER,' ','I','n','t','e','r','f','a','c','e',' ','1')
DEFINE_STRING_DESCRIPTOR(interface2String, 22, 'U','S','B',' ','T','e','s','t',' ','D','e','v','i','c','e',' ',TEST_DEVICE_LETTER,' ','P','o','r','t')
#ifdef USE_MS_OS_10
DEFINE_STRING_DESCRIPTOR(osString, 8, 'M','S','F','T','1','0','0', REQUEST_GET_MS_DESCRIPTOR)
#endif

uint16_t CODE * CODE usbStringDescriptors[] =
{
    languagesString,
    manufacturerString,
    productString,
    serialNumberStringDescriptor,
    interface0String,
    interface1String,
    interface2String
};

// See https://msdn.microsoft.com/en-us/library/ff540054.aspx
USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor =
{
    sizeof(USB_DESCRIPTOR_DEVICE),
    USB_DESCRIPTOR_TYPE_DEVICE,
#ifdef USE_MS_OS_20
    0x0201,                 // USB version: 2.0 with LPM ECN
#else
    0x0200,                 // USB version: 2.0
#endif
#ifdef COMPOSITE
    0xEF,                   // Class Code
    0x02,                   // Subclass code
    0x01,                   // Protocol code
#else
    0xFF,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
#endif
    USB_EP0_PACKET_SIZE,    // Max packet size for Endpoint 0
    USB_VENDOR_ID_POLOLU,   // Vendor ID
#ifdef TEST_DEVICE_B
    0xDA02,                 // Product ID: USB Test Device B
#else
    0xDA01,                 // Product ID: USB Test Device A
#endif
    0x0007,                 // Device release number in BCD format
    1,                      // Index of Manufacturer String Descriptor
    2,                      // Index of Product String Descriptor
    3,                      // Index of Serial Number String Descriptor
    1                       // Number of possible configurations.
};

#ifdef COMPOSITE

// Composite configuration descriptor.
struct CONFIG1 {
    USB_DESCRIPTOR_CONFIGURATION configuration;
    USB_DESCRIPTOR_INTERFACE nativeInterface0;
    USB_DESCRIPTOR_ENDPOINT adcDataIn;
    USB_DESCRIPTOR_ENDPOINT cmdOut;
    USB_DESCRIPTOR_ENDPOINT cmdIn;
    USB_DESCRIPTOR_INTERFACE nativeInterface1;
    USB_DESCRIPTOR_INTERFACE_ASSOCIATION portFunction;
    USB_DESCRIPTOR_INTERFACE portCommunicationInterface;
    uint8_t portClassSpecific[19];
    USB_DESCRIPTOR_ENDPOINT portNotificationElement;
    USB_DESCRIPTOR_INTERFACE portDataInterface;
    USB_DESCRIPTOR_ENDPOINT portDataOut;
    USB_DESCRIPTOR_ENDPOINT portDataIn;
} usbConfigurationDescriptor =
{
    {
        sizeof(USB_DESCRIPTOR_CONFIGURATION),
        USB_DESCRIPTOR_TYPE_CONFIGURATION,
        sizeof(struct CONFIG1),  // wTotalLength
        4,                       // bNumInterfaces
        1,                       // bConfigurationValue
        0,                       // STRING: iConfiguration
        0xC0,                    // bmAttributes: self power capable
        50,                      // bMaxPower (in units of 2 mA)
    },

    {
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        NATIVE_INTERFACE_0,  // bInterfaceNumber
        0,                   // bAlternateSetting
        3,                   // bNumEndpoints
        0xFF,                // bInterfaceClass: Vendor Specific
        0x00,                // bInterfaceSubClass
        0x00,                // bInterfaceProtocol
        4                    // STRING: iInterface
    },

    {
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | ADC_DATA_ENDPOINT,
        USB_TRANSFER_TYPE_INTERRUPT,
        ADC_DATA_PACKET_SIZE,
        1,
    },

    {
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_OUT | CMD_ENDPOINT,
        USB_TRANSFER_TYPE_BULK,
        CMD_PACKET_SIZE,
        0,
    },

    {
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | CMD_ENDPOINT,
        USB_TRANSFER_TYPE_BULK,
        CMD_PACKET_SIZE,
        0,
    },

    {
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        NATIVE_INTERFACE_1,  // bInterfaceNumber
        0,                   // bAlternateSetting
        0,                   // bNumEndpoints
        0xFF,                // bInterfaceClass: Vendor Specific
        0x00,                // bInterfaceSubClass
        0x00,                // bInterfaceProtocol
        5                    // STRING: iInterface
    },

    { // Port Interface Association Descriptor
        sizeof(USB_DESCRIPTOR_INTERFACE_ASSOCIATION),
        USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
        CDC_CONTROL_INTERFACE,  // first interface number
        2,                      // interface count
        CDC_CLASS,              // class
        CDC_SUBCLASS_ACM,       // subclass
        CDC_PROTOCOL_V250,      // protocol (enables automatic Linux support)
        6,                      // STRING
    },

    { // Communications Interface: Defines a virtual COM port.
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        CDC_CONTROL_INTERFACE,  // bInterfaceNumber
        0,                      // bAlternateSetting
        1,                      // bNumEndpoints
        CDC_CLASS,              // bInterfaceClass
        CDC_SUBCLASS_ACM,       // bInterfaceSubClass
        CDC_PROTOCOL_V250,      // bInterfaceProtocol
        0                       // STRING: iInterface
    },
    { // CDC Class-Specific Descriptors describing the virtual COM port.
        5,
    	CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
    	CDC_DESCRIPTOR_SUBTYPE_HEADER,
    	0x20,0x01,  // bcdCDC.  We conform to CDC 1.20

        4,
        CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
        CDC_DESCRIPTOR_SUBTYPE_ABSTRACT_CONTROL_MANAGEMENT,
    	2,  // bmCapabilities.  See USBPSTN1.2 Table 4.

    	5,
    	CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
    	CDC_DESCRIPTOR_SUBTYPE_UNION,
    	CDC_CONTROL_INTERFACE,  // index of the control interface
    	CDC_DATA_INTERFACE,     // index of the subordinate interface

    	5,
    	CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
    	CDC_DESCRIPTOR_SUBTYPE_CALL_MANAGEMENT,
    	0x00,  // bmCapabilities.  USBPSTN1.2 Table 3.
    	CDC_DATA_INTERFACE
    },
    { // USB Command Port notification endpoint.
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | CDC_NOTIFICATION_ENDPOINT,  // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                          // bmAttributes
        CDC_NOTIFICATION_PACKET_SIZE,                         // wMaxPacketSize
        1,                                                    // bInterval
    },
    { // Data interface: for sending data on the virtual COM port.
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        CDC_DATA_INTERFACE,             // bInterfaceNumber
        0,                              // bAlternateSetting
        2,                              // bNumEndpoints
        CDC_DATA_INTERFACE_CLASS,       // bInterfaceClass
        0,                              // bInterfaceSubClass
        0,                              // bInterfaceProtocol
        0                               // STIRNG: iInterface
    },
    { // Command port data OUT.
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_OUT | CDC_DATA_ENDPOINT,  // bEndpointAddress
        USB_TRANSFER_TYPE_BULK,                        // bmAttributes
        CDC_OUT_PACKET_SIZE,                           // wMaxPacketSize
        0,                                             // bInterval
    },
    { // Command port data IN.
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | CDC_DATA_ENDPOINT,  // bEndpointAddress
        USB_TRANSFER_TYPE_BULK,                       // bmAttributes
        CDC_IN_PACKET_SIZE,                           // wMaxPacketSize
        0,                                            // bInterval
    },
};

#else

// Non-composite configuration descriptor
struct CONFIG1 {
    USB_DESCRIPTOR_CONFIGURATION configuration;
    USB_DESCRIPTOR_INTERFACE nativeInterface0;
    USB_DESCRIPTOR_ENDPOINT adcDataIn;
} usbConfigurationDescriptor =
{
    {
        sizeof(USB_DESCRIPTOR_CONFIGURATION),
        USB_DESCRIPTOR_TYPE_CONFIGURATION,
        sizeof(struct CONFIG1),  // wTotalLength
        1,                       // bNumInterfaces
        1,                       // bConfigurationValue
        0,                       // STRING: iConfiguration
        0xC0,                    // bmAttributes: self power capable
        50,                      // bMaxPower (in units of 2 mA)
    },

    {
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        NATIVE_INTERFACE_0,  // bInterfaceNumber
        0,                   // bAlternateSetting
        1,                   // bNumEndpoints
        0xFF,                // bInterfaceClass: Vendor Specific
        0x00,                // bInterfaceSubClass
        0x00,                // bInterfaceProtocol
        4                    // STRING: iInterface
    },

    {
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | ADC_DATA_ENDPOINT,
        USB_TRANSFER_TYPE_INTERRUPT,
        ADC_DATA_PACKET_SIZE,
        1,
    },
};
#endif

#ifdef USE_MS_OS_10

XDATA uint8 compatIdDescriptor[0x28] =
{
    0x28, 0x00, 0x00, 0x00,    // dwLength
    0x00, 0x01,                // bcdVersion: 1.00
    0x04, 0x00,                // wIndex: Compatibility ID
    0x01,                      // bCount (number of sections)
    0x00, 0x00, 0x00, 0x00,    // reserved
    0x00, 0x00, 0x00,          // reserved
    0x00,                      // bFirstInterfaceNumber
    0x01,                      // reserved
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,        // compatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // subCompatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              // reserved
};

XDATA uint8 extendedPropertiesDescriptor[0x92] =
{
    0x92, 0x00, 0x00, 0x00,    // dwLength
    0x00, 0x01,                // bcdVersion: 1.00
    0x05, 0x00,                // wIndex: extended properties
    0x01, 0x00,                // wCount (number of sections)
    0x88, 0x00, 0x00, 0x00,    // dwSize of first section
    0x07, 0x00, 0x00, 0x00,    // dwPropertyDataType: REG_MULTI_SZ
    0x2a, 0x00,                // wPropertyNameLength
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,
    'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    0x50, 0x00, 0x00, 0x00,    // dwPropertyDataLength
    '{',0,'9',0,'9',0,'c',0,'4',0,'b',0,'b',0,'b',0,'0',0,'-',0,
    'e',0,'9',0,'2',0,'5',0,'-',0,'4',0,'3',0,'9',0,'7',0,'-',0,
    'a',0,'f',0,'e',0,'e',0,'-',0,'9',0,'8',0,'1',0,'c',0,'d',0,
    '0',0,'7',0,'0',0,'2',0,'1',0,'6',0,'3',0,'}',0,0,0,0,0,
};

#endif

#ifdef USE_MS_OS_20

#ifdef COMPOSITE

#define MS_OS_20_LENGTH 0xB2

// Micrsoft OS 2.0 Descriptor Set for a composite device.
XDATA uint8 msOs20DescriptorSet[MS_OS_20_LENGTH] =
{
    // Microsoft OS 2.0 Descriptor Set header (Table 10)
    0x0A, 0x00,  // wLength
    MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00,
    0x00, 0x00, 0x03, 0x06,  // dwWindowsVersion: Windows 8.1 (NTDDI_WINBLUE)
    MS_OS_20_LENGTH, 0x00,  // wTotalLength

    // Microsoft OS 2.0 configuration subset (Table 11)
    0x08, 0x00,  // wLength of this header
    MS_OS_20_SUBSET_HEADER_CONFIGURATION, 0x00,  // wDescriptorType
    0,            // configuration index
    0x00,         // bReserved
    0xA8, 0x00,   // wTotalLength of this subset

    // Microsoft OS 2.0 function subset header (Table 12)
    0x08, 0x00,  // wLength
    MS_OS_20_SUBSET_HEADER_FUNCTION, 0x00,  // wDescriptorType
    0,           // bFirstInterface
    0x00,        // bReserved,
    0xA0, 0x00,  // wSubsetLength

    // Microsoft OS 2.0 compatible ID descriptor (Table 13)
    0x14, 0x00,                                      // wLength
    MS_OS_20_FEATURE_COMPATIBLE_ID, 0x00,            // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,        // compatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // subCompatibleID

    // Microsoft OS 2.0 registry property descriptor (Table 14)
    0x84, 0x00,   // wLength
    MS_OS_20_FEATURE_REG_PROPERTY, 0x00,
    0x07, 0x00,   // wPropertyDataType: REG_MULTI_SZ
    0x2a, 0x00,   // wPropertyNameLength
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,
    'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    0x50, 0x00,   // wPropertyDataLength
    '{',0,'9',0,'9',0,'c',0,'4',0,'b',0,'b',0,'b',0,'0',0,'-',0,
    'e',0,'9',0,'2',0,'5',0,'-',0,'4',0,'3',0,'9',0,'7',0,'-',0,
    'a',0,'f',0,'e',0,'e',0,'-',0,'9',0,'8',0,'1',0,'c',0,'d',0,
    '0',0,'7',0,'0',0,'2',0,'1',0,'6',0,'3',0,'}',0,0,0,0,0,
};

#else

#define MS_OS_20_LENGTH 0xA2

// Micrsoft OS 2.0 Descriptor Set for a non-composite device.
XDATA uint8 msOs20DescriptorSet[MS_OS_20_LENGTH] =
{
    // Microsoft OS 2.0 Descriptor Set header (Table 10)
    0x0A, 0x00,  // wLength
    MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00,
    0x00, 0x00, 0x03, 0x06,  // dwWindowsVersion: Windows 8.1 (NTDDI_WINBLUE)
    MS_OS_20_LENGTH, 0x00,  // wTotalLength

    // Microsoft OS 2.0 compatible ID descriptor (Table 13)
    0x14, 0x00,                                      // wLength
    MS_OS_20_FEATURE_COMPATIBLE_ID, 0x00,            // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,        // compatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // subCompatibleID

    // Microsoft OS 2.0 registry property descriptor (Table 14)
    0x84, 0x00,   // wLength
    MS_OS_20_FEATURE_REG_PROPERTY, 0x00,
    0x07, 0x00,   // wPropertyDataType: REG_MULTI_SZ
    0x2a, 0x00,   // wPropertyNameLength
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,
    'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    0x50, 0x00,   // wPropertyDataLength
    '{',0,'9',0,'9',0,'c',0,'4',0,'b',0,'b',0,'b',0,'0',0,'-',0,
    'e',0,'9',0,'2',0,'5',0,'-',0,'4',0,'3',0,'9',0,'7',0,'-',0,
    'a',0,'f',0,'e',0,'e',0,'-',0,'9',0,'8',0,'1',0,'c',0,'d',0,
    '0',0,'7',0,'0',0,'2',0,'1',0,'6',0,'3',0,'}',0,0,0,0,0,
};

#endif

XDATA uint8 bosDescriptor[0x21] =
{
    0x05,       // bLength of this descriptor
    USB_DESCRIPTOR_TYPE_BOS,
    0x21, 0x00, // wLength
    0x01,       // bNumDeviceCaps

    0x1C,       // bLength of this first device capability descriptor
    USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
    USB_DEVICE_CAPABILITY_TYPE_PLATFORM,
    0x00,       // bReserved
    // Microsoft OS 2.0 descriptor platform capability UUID
    // from Microsoft OS 2.0 Descriptors,  Table 3.
    0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

    0x00, 0x00, 0x03, 0x06,  // dwWindowsVersion: Windows 8.1 (NTDDI_WINBLUE)
    MS_OS_20_LENGTH, 0x00,              // wMSOSDescriptorSetTotalLength
    REQUEST_GET_MS_DESCRIPTOR,
    0,                       // bAltEnumCode
};

#endif

// Make a buffer that takes 4 packets to transfer.
XDATA uint8_t dataBuffer[USB_EP0_PACKET_SIZE * 3 + 4];

bool adcPaused = 0;
uint16_t adcPauseStartTime;
uint16_t adcPauseDuration;

bool yellowOn = 0;

void usbCallbackInitEndpoints()
{
    usbInitEndpointIn(ADC_DATA_ENDPOINT, ADC_DATA_PACKET_SIZE);
    usbInitEndpointOut(CMD_ENDPOINT, CMD_PACKET_SIZE);
    usbInitEndpointIn(CMD_ENDPOINT, CMD_PACKET_SIZE);
}

void usbCallbackSetupHandler()
{
    switch(usbSetupPacket.bRequest)
    {
    case REQUEST_GET_MS_DESCRIPTOR:
        #ifdef USE_MS_OS_20
        if (usbSetupPacket.bmRequestType == 0xC0 &&
            usbSetupPacket.wIndex == MS_OS_20_DESCRIPTOR_INDEX)
        {
            // Request for Microsoft OS 2.0 Descriptor Set.
            usbControlRead(sizeof(msOs20DescriptorSet),
                (uint8 XDATA *)&msOs20DescriptorSet);
        }
        #endif

        #ifdef USE_MS_OS_10
        if (usbSetupPacket.bmRequestType == 0xC0 &&
            usbSetupPacket.wIndex == 4)
        {
            // Request for an extended compat ID, as defined in Microsoft
            // OS Descriptors 1.0.  This descriptor applies to the whole
            // device, so we expect wValue to be 0 here but we do not
            // check it.
            usbControlRead(sizeof(compatIdDescriptor),
                (uint8 XDATA *)&compatIdDescriptor);
        }

        if (usbSetupPacket.bmRequestType == 0xC1 &&
            usbSetupPacket.wValue == 0 &&
            usbSetupPacket.wIndex == 5)
        {
            // Request for extended properties on interface 0.
            usbControlRead(sizeof(extendedPropertiesDescriptor),
                (uint8 XDATA *)&extendedPropertiesDescriptor);
        }
        #endif
        return;

    case 0x90:  // Set LED
        yellowOn = usbSetupPacket.wValue & 1;
        usbControlAcknowledge();
        return;

    case 0x91:  // Read buffer
        // The length of the device's response will be equal to wIndex
        // so this requrest can be used to simulate what happens when
        // the device returns less data than expected.
        if (usbSetupPacket.wLength > sizeof(dataBuffer))
        {
            return;  // bad size
        }
        if (usbSetupPacket.wIndex > usbSetupPacket.wLength)
        {
            return;  // bad size
        }
        delayMs(usbSetupPacket.wValue);
        usbControlRead(usbSetupPacket.wIndex, (XDATA uint8_t *)&dataBuffer);
        return;

    case 0x92:  // Write buffer
        if (usbSetupPacket.wLength > sizeof(dataBuffer))
        {
            return;  // bad size
        }
        delayMs(usbSetupPacket.wValue);

        if (usbSetupPacket.wLength > 0)
        {
            usbControlWrite(usbSetupPacket.wLength, (XDATA uint8_t *)&dataBuffer);
        }
        else
        {
            usbControlAcknowledge();
        }
        return;

    case 0xA0:  // Pause or unpause the ADC data stream.
        if (usbSetupPacket.wValue == 0)
        {
            adcPaused = 0;
        }
        else
        {
            adcPaused = 1;
            adcPauseStartTime = getMs();
            adcPauseDuration = usbSetupPacket.wValue;
        }
        usbControlAcknowledge();
        return;
    }
}

void usbCallbackClassDescriptorHandler()
{
#ifdef USE_MS_OS_10
    if (usbSetupPacket.wValue == 0x03EE)
    {
        // Microsoft OS String descriptor
        usbControlRead(sizeof(osString), (uint8 XDATA *)&osString);
    }
#endif

#ifdef USE_MS_OS_20
    if (usbSetupPacket.wValue == 0x0f00)
    {
        // BOS Descriptor
        usbControlRead(sizeof(bosDescriptor), bosDescriptor);
    }
#endif
}

void usbCallbackControlWriteHandler()
{
}

// Sends ADC data to the computer with a USB endpoint.
void adcDataTx()
{
    if (usbDeviceState != USB_STATE_CONFIGURED)
    {
        // We have not reached the Configured state yet, so we should not be
        // touching the non-zero endpoints.
        return;
    }

    if (adcPaused)
    {
        if ((uint16_t)(getMs() - adcPauseStartTime) >= adcPauseDuration)
        {
            adcPaused = 0;
        }
        else
        {
            return;
        }
    }

    USBINDEX = ADC_DATA_ENDPOINT;

    if (!(USBCSIL & USBCSIL_INPKT_RDY))
    {
        uint16_t reading;

        // There is buffer space available, so queue up an IN
        // packet with some stuff in it.

        // Send the USB frame number.
        ADC_DATA_FIFO = USBFRML;
        ADC_DATA_FIFO = USBFRMH;

        // Send the ADC reading.
        reading = adcRead(5); // P0_5
        ADC_DATA_FIFO = reading & 0xFF;
        ADC_DATA_FIFO = reading >> 8 & 0xFF;

        // Send a constant byte so that we have some data that can actually be
        // checked for correctness in an automated test.
        ADC_DATA_FIFO = 0xAB;

        USBCSIL |= USBCSIL_INPKT_RDY;

        // Notify the USB library that some activity has occurred.
        usbActivityFlag = 1;
    }
}

void cmdService()
{
    uint8_t count, cmd, i;
    uint16_t delay;

    if (usbDeviceState != USB_STATE_CONFIGURED) { return; }

    USBINDEX = CMD_ENDPOINT;
    if (!(USBCSOL & USBCSOL_OUTPKT_RDY))
    {
        // No command packet available right now.
        return;
    }

    count = USBCNTL;

    if (count == 0)
    {
        // Empty packet
        dataBuffer[0] = 0x66;
    }

    if (count >= 2)
    {
      cmd = CMD_FIFO;

      // Command 0x92: Set a byte in dataBuffer
      if (cmd == 0x92)
      {
          dataBuffer[0] = CMD_FIFO;
      }

      // Command 0xDE: Delay
      if (cmd == 0xDE)
      {
          delay = CMD_FIFO;
          delay += CMD_FIFO << 8;
          delayMs(delay);
      }
    }

    for (i = 0; i < count; i++)
    {
      CMD_FIFO;
    }

    USBCSOL &= ~USBCSOL_OUTPKT_RDY;  // Done with this packet.
}

void main()
{
    uint8_t x = 0;
    setDigitalInput(0, 1);
    systemInit();
    usbInit();
    while(1)
    {
        boardService();
        usbShowStatusWithGreenLed();
        usbPoll();
        adcDataTx();
        cmdService();
        LED_YELLOW(yellowOn);

        if (!isPinHigh(0))
        {
            boardStartBootloader();
        }
    }
}

