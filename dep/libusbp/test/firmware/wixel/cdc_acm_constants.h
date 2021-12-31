#pragma once

// USB Class Codes
#define CDC_CLASS 2                  // (CDC 1.20 Section 4.1: Communications Device Class Code).
#define CDC_DATA_INTERFACE_CLASS 0xA // (CDC 1.20 Section 4.5: Data Class Interface Codes).

// USB Subclass Codes
#define CDC_SUBCLASS_ACM  2           // (CDC 1.20 Section 4.3: Communications Class Subclass Codes).  Refer to USBPSTN1.2.

// USB Protocol Codes
#define CDC_PROTOCOL_V250 1          // (CDC 1.20 Section 4.4: Communications Class Protocol Codes).

// USB Descriptor types from CDC 1.20 Section 5.2.3, Table 12
#define CDC_DESCRIPTOR_TYPE_CS_INTERFACE 0x24
#define CDC_DESCRIPTOR_TYPE_CS_ENDPOINT  0x25

// USB Descriptor sub-types from CDC 1.20 Table 13: bDescriptor SubType in Communications Class Functional Descriptors
#define CDC_DESCRIPTOR_SUBTYPE_HEADER                       0
#define CDC_DESCRIPTOR_SUBTYPE_CALL_MANAGEMENT              1
#define CDC_DESCRIPTOR_SUBTYPE_ABSTRACT_CONTROL_MANAGEMENT  2
#define CDC_DESCRIPTOR_SUBTYPE_UNION                        6

// Request Codes from CDC 1.20 Section 6.2: Management Element Requests.
#define ACM_GET_ENCAPSULATED_RESPONSE 0
#define ACM_SEND_ENCAPSULATED_COMMAND 1

// Request Codes from PSTN 1.20 Table 13.
#define ACM_REQUEST_SET_LINE_CODING 0x20
#define ACM_REQUEST_GET_LINE_CODING 0x21
#define ACM_REQUEST_SET_CONTROL_LINE_STATE 0x22

// Notification Codes from PSTN 1.20 Table 30.
#define ACM_NOTIFICATION_RESPONSE_AVAILABLE 0x01
#define ACM_NOTIFICATION_SERIAL_STATE 0x20
