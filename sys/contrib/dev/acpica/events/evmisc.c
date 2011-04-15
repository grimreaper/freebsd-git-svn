/******************************************************************************
 *
 * Module Name: evmisc - Miscellaneous event manager support functions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2011, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <contrib/dev/acpica/include/acpi.h>
#include <contrib/dev/acpica/include/accommon.h>
#include <contrib/dev/acpica/include/acevents.h>
#include <contrib/dev/acpica/include/acnamesp.h>

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evmisc")


/* Local prototypes */

static void ACPI_SYSTEM_XFACE
AcpiEvNotifyDispatch (
    void                    *Context);


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvIsNotifyObject
 *
 * PARAMETERS:  Node            - Node to check
 *
 * RETURN:      TRUE if notifies allowed on this object
 *
 * DESCRIPTION: Check type of node for a object that supports notifies.
 *
 *              TBD: This could be replaced by a flag bit in the node.
 *
 ******************************************************************************/

BOOLEAN
AcpiEvIsNotifyObject (
    ACPI_NAMESPACE_NODE     *Node)
{
    switch (Node->Type)
    {
    case ACPI_TYPE_DEVICE:
    case ACPI_TYPE_PROCESSOR:
    case ACPI_TYPE_THERMAL:
        /*
         * These are the ONLY objects that can receive ACPI notifications
         */
        return (TRUE);

    default:
        return (FALSE);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvQueueNotifyRequest
 *
 * PARAMETERS:  Node            - NS node for the notified object
 *              NotifyValue     - Value from the Notify() request
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvQueueNotifyRequest (
    ACPI_NAMESPACE_NODE     *Node,
    UINT32                  NotifyValue)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj = NULL;
    ACPI_GENERIC_STATE      *NotifyInfo;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_NAME (EvQueueNotifyRequest);


    /*
     * For value 3 (Ejection Request), some device method may need to be run.
     * For value 2 (Device Wake) if _PRW exists, the _PS0 method may need
     *   to be run.
     * For value 0x80 (Status Change) on the power button or sleep button,
     *   initiate soft-off or sleep operation?
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Dispatching Notify on [%4.4s] Node %p Value 0x%2.2X (%s)\n",
        AcpiUtGetNodeName (Node), Node, NotifyValue,
        AcpiUtGetNotifyName (NotifyValue)));

    /* Get the notify object attached to the NS Node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        /* We have the notify object, Get the right handler */

        switch (Node->Type)
        {
        /* Notify allowed only on these types */

        case ACPI_TYPE_DEVICE:
        case ACPI_TYPE_THERMAL:
        case ACPI_TYPE_PROCESSOR:

            if (NotifyValue <= ACPI_MAX_SYS_NOTIFY)
            {
                HandlerObj = ObjDesc->CommonNotify.SystemNotify;
            }
            else
            {
                HandlerObj = ObjDesc->CommonNotify.DeviceNotify;
            }
            break;

        default:

            /* All other types are not supported */

            return (AE_TYPE);
        }
    }

    /*
     * If there is any handler to run, schedule the dispatcher.
     * Check for:
     * 1) Global system notify handler
     * 2) Global device notify handler
     * 3) Per-device notify handler
     */
    if ((AcpiGbl_SystemNotify.Handler &&
            (NotifyValue <= ACPI_MAX_SYS_NOTIFY)) ||
        (AcpiGbl_DeviceNotify.Handler &&
            (NotifyValue > ACPI_MAX_SYS_NOTIFY))  ||
        HandlerObj)
    {
        NotifyInfo = AcpiUtCreateGenericState ();
        if (!NotifyInfo)
        {
            return (AE_NO_MEMORY);
        }

        if (!HandlerObj)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                "Executing system notify handler for Notify (%4.4s, %X) "
                "node %p\n",
                AcpiUtGetNodeName (Node), NotifyValue, Node));
        }

        NotifyInfo->Common.DescriptorType = ACPI_DESC_TYPE_STATE_NOTIFY;
        NotifyInfo->Notify.Node = Node;
        NotifyInfo->Notify.Value = (UINT16) NotifyValue;
        NotifyInfo->Notify.HandlerObj = HandlerObj;

        Status = AcpiOsExecute (
                    OSL_NOTIFY_HANDLER, AcpiEvNotifyDispatch, NotifyInfo);
        if (ACPI_FAILURE (Status))
        {
            AcpiUtDeleteGenericState (NotifyInfo);
        }
    }
    else
    {
        /* There is no notify handler (per-device or system) for this device */

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "No notify handler for Notify (%4.4s, %X) node %p\n",
            AcpiUtGetNodeName (Node), NotifyValue, Node));
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvNotifyDispatch
 *
 * PARAMETERS:  Context         - To be passed to the notify handler
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 ******************************************************************************/

static void ACPI_SYSTEM_XFACE
AcpiEvNotifyDispatch (
    void                    *Context)
{
    ACPI_GENERIC_STATE      *NotifyInfo = (ACPI_GENERIC_STATE *) Context;
    ACPI_NOTIFY_HANDLER     GlobalHandler = NULL;
    void                    *GlobalContext = NULL;
    ACPI_OPERAND_OBJECT     *HandlerObj;


    ACPI_FUNCTION_ENTRY ();


    /*
     * We will invoke a global notify handler if installed. This is done
     * _before_ we invoke the per-device handler attached to the device.
     */
    if (NotifyInfo->Notify.Value <= ACPI_MAX_SYS_NOTIFY)
    {
        /* Global system notification handler */

        if (AcpiGbl_SystemNotify.Handler)
        {
            GlobalHandler = AcpiGbl_SystemNotify.Handler;
            GlobalContext = AcpiGbl_SystemNotify.Context;
        }
    }
    else
    {
        /* Global driver notification handler */

        if (AcpiGbl_DeviceNotify.Handler)
        {
            GlobalHandler = AcpiGbl_DeviceNotify.Handler;
            GlobalContext = AcpiGbl_DeviceNotify.Context;
        }
    }

    /* Invoke the system handler first, if present */

    if (GlobalHandler)
    {
        GlobalHandler (NotifyInfo->Notify.Node, NotifyInfo->Notify.Value,
            GlobalContext);
    }

    /* Now invoke the per-device handler, if present */

    HandlerObj = NotifyInfo->Notify.HandlerObj;
    if (HandlerObj)
    {
        HandlerObj->Notify.Handler (NotifyInfo->Notify.Node,
            NotifyInfo->Notify.Value,
            HandlerObj->Notify.Context);
    }

    /* All done with the info object */

    AcpiUtDeleteGenericState (NotifyInfo);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiEvTerminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Disable events and free memory allocated for table storage.
 *
 ******************************************************************************/

void
AcpiEvTerminate (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvTerminate);


    if (AcpiGbl_EventsInitialized)
    {
        /*
         * Disable all event-related functionality. In all cases, on error,
         * print a message but obviously we don't abort.
         */

        /* Disable all fixed events */

        for (i = 0; i < ACPI_NUM_FIXED_EVENTS; i++)
        {
            Status = AcpiDisableEvent (i, 0);
            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR ((AE_INFO,
                    "Could not disable fixed event %u", (UINT32) i));
            }
        }

        /* Disable all GPEs in all GPE blocks */

        Status = AcpiEvWalkGpeList (AcpiHwDisableGpeBlock, NULL);

        /* Remove SCI handler */

        Status = AcpiEvRemoveSciHandler ();
        if (ACPI_FAILURE(Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not remove SCI handler"));
        }

        Status = AcpiEvRemoveGlobalLockHandler ();
        if (ACPI_FAILURE(Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not remove Global Lock handler"));
        }
    }

    /* Deallocate all handler objects installed within GPE info structs */

    Status = AcpiEvWalkGpeList (AcpiEvDeleteGpeHandlers, NULL);

    /* Return to original mode if necessary */

    if (AcpiGbl_OriginalMode == ACPI_SYS_MODE_LEGACY)
    {
        Status = AcpiDisable ();
        if (ACPI_FAILURE (Status))
        {
            ACPI_WARNING ((AE_INFO, "AcpiDisable failed"));
        }
    }
    return_VOID;
}
