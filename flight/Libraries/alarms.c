/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotLibraries OpenPilot System Libraries
 * @brief OpenPilot System libraries are available to all OP modules.
 * @{
 * @file       alarms.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Library for setting and clearing system alarms
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <openpilot.h>
#include "inc/alarms.h"

// Private constants

// Private types

// Private variables
static xSemaphoreHandle lock;

// Private functions
static int32_t hasSeverity(SystemAlarmsAlarmOptions severity);

/**
 * Initialize the alarms library
 */
int32_t AlarmsInitialize(void)
{
    SystemAlarmsInitialize();

    lock = xSemaphoreCreateRecursiveMutex();
    //do not change the default states of the alarms, let the init code generated by the uavobjectgenerator handle that
    //AlarmsClearAll();
    //AlarmsDefaultAll();
    return 0;
}

/**
 * Set an alarm
 * @param alarm The system alarm to be modified
 * @param severity The alarm severity
 * @return 0 if success, -1 if an error
 */
int32_t AlarmsSet(SystemAlarmsAlarmElem alarm, SystemAlarmsAlarmOptions severity)
{
    SystemAlarmsData alarms;

    // Check that this is a valid alarm
    if (alarm >= SYSTEMALARMS_ALARM_NUMELEM) {
        return -1;
    }

    // Lock
    if (xSemaphoreTakeRecursive(lock, portMAX_DELAY) == pdFALSE) {
        return -1;
    }

    // Read alarm and update its severity only if it was changed
    SystemAlarmsGet(&alarms);
    if (alarms.Alarm[alarm] != severity) {
        alarms.Alarm[alarm] = severity;
        SystemAlarmsSet(&alarms);
    }

    // Release lock
    xSemaphoreGiveRecursive(lock);
    return 0;

}

/**
 * Set an Extended Alarm
 * @param alarm The system alarm to be modified
 * @param severity The alarm severity
 * @param status: the Extended alarm status field
 * @param subStatus: the Extended alarm substatus field
 * @return 0 if success, -1 if an error
 */
int32_t ExtendedAlarmsSet(SystemAlarmsAlarmElem alarm, SystemAlarmsAlarmOptions severity, uint8_t status, uint8_t subStatus)
{
    SystemAlarmsData alarms;

    // Check that this is a valid alarm
    if (alarm >= SYSTEMALARMS_EXTENDEDALARMSTATUS_NUMELEM) {
        return -1;
    }

    // Lock
    if (xSemaphoreTakeRecursive(lock, portMAX_DELAY) == pdFALSE) {
        return -1;
    }

    // Read alarm and update its severity only if it was changed
    SystemAlarmsGet(&alarms);
    if (alarms.Alarm[alarm] != severity) {
        alarms.ExtendedAlarmStatus[alarm] = status;
        alarms.ExtendedAlarmSubStatus[alarm] = subStatus;
        alarms.Alarm[alarm] = severity;
        SystemAlarmsSet(&alarms);
    }

    // Release lock
    xSemaphoreGiveRecursive(lock);
    return 0;
}

/**
 * Get an alarm
 * @param alarm The system alarm to be read
 * @return Alarm severity
 */
SystemAlarmsAlarmOptions AlarmsGet(SystemAlarmsAlarmElem alarm)
{
    SystemAlarmsData alarms;

    // Check that this is a valid alarm
    if (alarm >= SYSTEMALARMS_ALARM_NUMELEM) {
        return 0;
    }

    // Read alarm
    SystemAlarmsGet(&alarms);
    return alarms.Alarm[alarm];
}

/**
 * Set an alarm to it's default value
 * @param alarm The system alarm to be modified
 * @return 0 if success, -1 if an error
 */
int32_t AlarmsDefault(SystemAlarmsAlarmElem alarm)
{
    return AlarmsSet(alarm, SYSTEMALARMS_ALARM_DEFAULT);
}

/**
 * Default all alarms
 */
void AlarmsDefaultAll()
{
    uint32_t n;
    for (n = 0; n < SYSTEMALARMS_ALARM_NUMELEM; ++n) {
        AlarmsDefault(n);
    }
}

/**
 * Clear an alarm
 * @param alarm The system alarm to be modified
 * @return 0 if success, -1 if an error
 */
int32_t AlarmsClear(SystemAlarmsAlarmElem alarm)
{
    if (alarm < SYSTEMALARMS_EXTENDEDALARMSTATUS_NUMELEM) {
        return ExtendedAlarmsSet(alarm, SYSTEMALARMS_ALARM_OK, 0, 0);
    } else {
        return AlarmsSet(alarm, SYSTEMALARMS_ALARM_OK);
    }
}

/**
 * Clear all alarms
 */
void AlarmsClearAll()
{
    uint32_t n;
    for (n = 0; n < SYSTEMALARMS_ALARM_NUMELEM; ++n) {
        AlarmsClear(n);
    }
}

/**
 * Check if there are any alarms with the given or higher severity
 * @return 0 if no alarms are found, 1 if at least one alarm is found
 */
int32_t AlarmsHasWarnings()
{
    return hasSeverity(SYSTEMALARMS_ALARM_WARNING);
}

/**
 * Check if there are any alarms with error or higher severity
 * @return 0 if no alarms are found, 1 if at least one alarm is found
 */
int32_t AlarmsHasErrors()
{
    return hasSeverity(SYSTEMALARMS_ALARM_ERROR);
}


/**
 * Check if there are any alarms with critical or higher severity
 * @return 0 if no alarms are found, 1 if at least one alarm is found
 */
int32_t AlarmsHasCritical()
{
    return hasSeverity(SYSTEMALARMS_ALARM_CRITICAL);
}


/**
 * Check if there are any alarms with the given or higher severity
 * @return 0 if no alarms are found, 1 if at least one alarm is found
 */
static int32_t hasSeverity(SystemAlarmsAlarmOptions severity)
{
    SystemAlarmsData alarms;
    uint32_t n;

    // Lock
    xSemaphoreTakeRecursive(lock, portMAX_DELAY);

    // Read alarms
    SystemAlarmsGet(&alarms);

    // Go through alarms and check if any are of the given severity or higher
    for (n = 0; n < SYSTEMALARMS_ALARM_NUMELEM; ++n) {
        if (alarms.Alarm[n] >= severity) {
            xSemaphoreGiveRecursive(lock);
            return 1;
        }
    }

    // If this point is reached then no alarms found
    xSemaphoreGiveRecursive(lock);
    return 0;
}
/**
 * @}
 * @}
 */

