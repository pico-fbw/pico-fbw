#ifndef __STATUS_H
#define __STATUS_H

#include <stdint.h>

#include "fusion.h"

/// StatusSubsystem() provides an object-like interface for communicating status to the user
typedef struct StatusSubsystem {
	// // Required internal states
	fusion_status_t		previous;       ///< Previous status state - fusion_status_t is defined in fusion.h
	fusion_status_t		status;         ///< Current status
	fusion_status_t		next;           ///< Pending status change
	// Required methods
	ssSetStatus_t           *set;	        ///< change status immediately - no delay
	ssGetStatus_t           *get;	        ///< return status
	ssSetStatus_t           *queue;         ///< queue status change for next regular interval
	ssUpdateStatus_t        *update;        ///< make pending status active/visible
	ssUpdateStatus_t        *test ;         ///< unit test which simply increments to next state
	// application-specific internal variables
	uint8_t toggle;                      ///< This implementation can change LED color and have either solid/toggle
} StatusSubsystem ;

/// initializeStatusSubsystem() should be called once at startup to initialize the
/// data structure and to put hardware into the proper state for communicating status.
void initializeStatusSubsystem (
  StatusSubsystem *pStatus                      ///< pointer to the status subsystem
);

#endif /* __STATUS_H */
