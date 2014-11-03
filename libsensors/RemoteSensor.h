/*
 * Device File-based I/O Virtualization (DFV)
 * File: RemoteSensor.h
 *
 * Copyright (c) 2014 Rice University, Houston, TX, USA
 * All rights reserved.
 *
 * Authors: Ardalan Amiri Sani <arrdalan@gmail.com>
 *
 * Originally based on local sensor code. 
 *
 * Copyright (C) 2011 The Android Open-Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef REMOTE_SENSOR_H
#define REMOTE_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"

class RemoteSensor{
private:    
    struct pollfd *rPollFds; /* remote pollfd's */
    SensorBase **rSensors; /* remote sensors */
    int numRSensors;
    bool initialized;
    int init_remote_device();
        
public:
    RemoteSensor();
    ~RemoteSensor();
    int pollEvents(sensors_event_t *data, int count);
    bool remoteDevicePresent();
};
#endif /* REMOTE_SENSOR_H */
