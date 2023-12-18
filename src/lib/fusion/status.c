/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "../../sys/log.h"

#include "../../modes/aircraft.h"

#include "drivers/drivers.h"
#include "fconfig.h"

#include "status.h"

// Do an immediate status update
void ssSetStatusNow(StatusSubsystem *pStatus, fusion_status_t status)
{
    pStatus->status = status;
    pStatus->next = status;
    // If the status is severe enough, log it
    switch (status) {
        case NORMAL:
            aircraft.setAAHRSSafe(true);
            break;
        case SOFT_FAULT:
            log_message(WARNING, "Fusion fault!", 1000, 0, false);
            aircraft.setAAHRSSafe(false);
            break;
        case HARD_FAULT:
            log_message(ERROR, "Unrecoverable fusion fault!", 1000, 0, false);
            aircraft.setAAHRSSafe(false);
            break;
        default:
            break;
    }
}

// queue up a status change (which will take place at the next updateStatus)
void ssQueueStatus(StatusSubsystem *pStatus, fusion_status_t status)
{
    pStatus->next = status;
}

// promote any previously queued status update
void ssUpdateStatus(StatusSubsystem *pStatus)
{
    pStatus->previous = pStatus->status;
    ssSetStatusNow(pStatus, pStatus->next);
}

// make an immediate update to the system status
void ssSetStatus(StatusSubsystem *pStatus, fusion_status_t status)
{
    pStatus->next = status;
    ssUpdateStatus(pStatus);
}

// return the system status
fusion_status_t ssGetStatus(StatusSubsystem *pStatus)
{
    return pStatus->status;
}

/// initializeStatusSubsystem() should be called once at startup to initialize the
/// data structure and to put hardware into the proper state for communicating status.
void initializeStatusSubsystem(StatusSubsystem *pStatus)
{
    pStatus->set = ssSetStatus;
    pStatus->get = ssGetStatus;
    pStatus->queue = ssQueueStatus;
    pStatus->update = ssUpdateStatus;
    pStatus->previous = OFF;
    pStatus->set(pStatus, OFF);
    pStatus->queue(pStatus, OFF);
    pStatus->toggle = false;
}
