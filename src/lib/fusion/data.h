#ifndef __DATA_H
#define __DATA_H

#include "fusion.h"

#define MAX_LEN_SERIAL_OUTPUT_BUF   255  // larger than the nominal 124 byte size for outgoing packets

/// @name Control Port Function Type Definitions
/// "write" "stream" and "readCommands" provide three control functions visible at the main()
/// level.  These typedefs define the structure of those calls.
///@{
typedef int8_t (writePort_t) (SensorFusionGlobals *sfg);
typedef int8_t (readCommand_t) (SensorFusionGlobals *sfg);
typedef void (injectCommand_t) (SensorFusionGlobals *sfg, uint8_t input_buffer[], uint16_t nbytes);
typedef void (streamData_t)(SensorFusionGlobals *sfg);
///@}

/// \brief The ControlSubsystem encapsulates command and data streaming functions.
///
/// The ControlSubsystem encapsulates command and data streaming functions
/// for the library.  A C++-like typedef structure which includes executable methods
/// for the subsystem is defined here.
typedef struct ControlSubsystem {
	quaternion_type DefaultQuaternionPacketType;	// default quaternion transmitted at power on
	volatile quaternion_type QuaternionPacketType;	// quaternion type transmitted over UART
	volatile uint8_t AngularVelocityPacketOn;	// flag to enable angular velocity packet
	volatile uint8_t DebugPacketOn;			// flag to enable debug packet
	volatile uint8_t RPCPacketOn;			// flag to enable roll, pitch, compass packet
	volatile uint8_t AltPacketOn;			// flag to enable altitude packet
	volatile int8_t  AccelCalPacketOn;      // variable used to coordinate accelerometer calibration
    uint8_t         *serial_out_buf;        //buffer containing the output stream (data packet)
    uint16_t        bytes_to_send;          //how many bytes in output stream waiting to go out
    const void *serial_port;           //cast to Serial * and used to output to the serial port
    const void *tcp_client;            //cast to WiFiClient * and used to output to a connected TCP client

    writePort_t *write;  // function to write output buffer to the output(s)
    readCommand_t *readCommands;  // function to check for incoming commands and process them
    injectCommand_t *injectCommand;  // function that provides a command directly and processes it
    streamData_t *stream;  // function to create output data packets and place in buffer
} ControlSubsystem;

bool initializeIOSubsystem(
    ControlSubsystem *pComm, const void *serial_port,
    const void *tcp_client);  // Initialize structures, ports, etc.

//updates pointer to the TCP client. Call whenever new client connects or disconnects
void UpdateTCPClient(ControlSubsystem *pComm,void *tcp_client);

// Located in output_stream.c:
/// Called once per fusion cycle to stream information required by the NXP
/// Sensor Fusion Toolbox. Packet protocols are defined in the NXP Sensor Fusion
/// for Kinetis Product Development Kit User Guide.
void CreateOutgoingPackets(SensorFusionGlobals *sfg);

/// Located in control_input.c:
/// This function is responsible for decoding commands, which can arrive externally
/// (serial or WiFi) when sent by the NXP Sensor Fusion Toolbox, or by direct call.
/// This function sets the appropriate flags in the ControlSubsystem data structure,
/// or performs whatever other appropriate action called for by the command.
/// Packet protocols are defined in the NXP Sensor Fusion for Kinetis Product Development Kit User Guide.
void DecodeCommandBytes(SensorFusionGlobals *sfg, uint8_t input_buffer[], uint16_t nbytes);

/// Utility function used to place data in output buffer about to be transmitted via UART
void OutputBufAppendItem(uint8_t *pDest, uint16_t *pIndex, uint8_t *pSource, uint16_t iBytesToCopy);

#endif // __DATA_H
