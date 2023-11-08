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

#include "drivers/drivers.h"
#include "fconfig.h"

#include "status.h"

// TODO: redo status indicators for log subsystem
// also all the code here was breaking so I commented it...replace it!!

// Do an immediate status update
void ssSetStatusNow(StatusSubsystem *pStatus, fusion_status_t status)
{
    // pStatus->status = status;
    // pStatus->next = status;
    // while (status == HARD_FAULT) ; // Never return on hard fault
    // // while (status == SOFT_FAULT) ; // DEBUG ONLY Never return on soft fault
}

// Unit test for status sub-system
void ssTest(StatusSubsystem *pStatus)
{
    // switch (pStatus->status)
    // {
    //     case OFF:
    //         ssSetStatusNow(pStatus, INITIALIZING);
    //         break;
    //     case INITIALIZING:
    //         ssSetStatusNow(pStatus, LOWPOWER);
    //         break;
    //     case LOWPOWER:
    //         ssSetStatusNow(pStatus, NORMAL);
    //         break;
    //     case NORMAL:
    //         ssSetStatusNow(pStatus, RECEIVING_WIRED);
    //         break;
    //     case RECEIVING_WIRED:
    //         ssSetStatusNow(pStatus, RECEIVING_WIRELESS);
    //         break;
    //     case RECEIVING_WIRELESS:
    //         ssSetStatusNow(pStatus, SOFT_FAULT);
    //         break;
    //     case SOFT_FAULT:
    //         ssSetStatusNow(pStatus, HARD_FAULT);
    //         break;
    //     case HARD_FAULT:
    //         ssSetStatusNow(pStatus, OFF);
    //         break;
    // }
}


// queue up a status change (which will take place at the next updateStatus)
void ssQueueStatus(StatusSubsystem *pStatus, fusion_status_t status)
{
    // pStatus->next = status;
}

// promote any previously queued status update
void ssUpdateStatus(StatusSubsystem *pStatus)
{
    // pStatus->previous = pStatus->status;
    // ssSetStatusNow(pStatus, pStatus->next);
}

// make an immediate update to the system status
void ssSetStatus(StatusSubsystem *pStatus, fusion_status_t status)
{
    // pStatus->next = status;
    // ssUpdateStatus(pStatus);
}

// return the system status
fusion_status_t ssGetStatus(StatusSubsystem *pStatus)
{
    // return pStatus->status;
}

/// initializeStatusSubsystem() should be called once at startup to initialize the
/// data structure and to put hardware into the proper state for communicating status.
void initializeStatusSubsystem(StatusSubsystem *pStatus)
{
    // pStatus->set = ssSetStatus;
    // pStatus->get = ssGetStatus;
    // pStatus->queue = ssQueueStatus;
    // pStatus->update = ssUpdateStatus;
    // pStatus->test = ssTest;
    // pStatus->previous = OFF;
    // pStatus->set(pStatus, OFF);
    // pStatus->queue(pStatus, OFF);
    // pStatus->toggle = false;
}
