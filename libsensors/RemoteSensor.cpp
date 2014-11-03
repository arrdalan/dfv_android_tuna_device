/*
 * Device File-based I/O Virtualization (DFV)
 * File: RemoteSensor.cpp
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <pthread.h>
#include <sys/stat.h>

#include "RemoteSensor.h"
#include "MPLSensor.h"
#include "sensors.h"
#include <utils/dfv.h>

char *mpu_dev_name = "/dev/mpu2";
char *mpuirq_dev_name = "/dev/mpuirq2";
char *accelirq_dev_name = "/dev/accelirq2";
char *timerirq_dev_name = "/dev/timerirq2";

RemoteSensor::RemoteSensor()
    : numRSensors(0), initialized(false)
{    
    return;    
}

RemoteSensor::~RemoteSensor()
{
    return;
}

bool RemoteSensor::remoteDevicePresent()
{
	if (file_present(mpu_dev_name) && file_present(mpuirq_dev_name) &&
		file_present(accelirq_dev_name) && file_present(timerirq_dev_name))
		return true;

	return false;
}

/* Copied from struct sensors_poll_context_t from sensors.cpp. */
enum {
        mpl               = 0,  //all mpl entries must be consecutive and in this order
        mpl_accel,
        mpl_timer,
        light,
        proximity,
        pressure,
        numSensorDrivers,       // wake pipe goes here
        mpl_power,              //special handle for MPL pm interaction
        numFds,
};

/* Copied and modified from sensors_poll_context_t constructor in sensors.cpp. */
int RemoteSensor::init_remote_device()
{	
    numRSensors = 3;	
    rPollFds = (struct pollfd *) malloc(numRSensors * sizeof(struct pollfd));
    rSensors = (SensorBase **) malloc(numRSensors * sizeof(SensorBase *));
    		
    
    MPLSensor* p_mplsen = new MPLSensor(mpu_dev_name, mpuirq_dev_name,
    					accelirq_dev_name, timerirq_dev_name);
    
    
    p_mplsen->gbpt = (void *) p_mplsen;

    rSensors[mpl] = p_mplsen;
    rPollFds[mpl].fd = rSensors[mpl]->getFd();
    rPollFds[mpl].events = POLLIN;
    rPollFds[mpl].revents = 0;

    rSensors[mpl_accel] = rSensors[mpl];
    rPollFds[mpl_accel].fd = ((MPLSensor*)rSensors[mpl])->getAccelFd();
    rPollFds[mpl_accel].events = POLLIN;
    rPollFds[mpl_accel].revents = 0;
    p_mplsen->enable(ID_A, 1);

    rSensors[mpl_timer] = rSensors[mpl];
    rPollFds[mpl_timer].fd = ((MPLSensor*)rSensors[mpl])->getTimerFd();
    rPollFds[mpl_timer].events = POLLIN;
    rPollFds[mpl_timer].revents = 0;
    
    initialized = true;
    
    return 0;
}

int RemoteSensor::pollEvents(sensors_event_t* data, int count)
{
    //FUNC_LOG;
    int nbEvents = 0;
    int n = 0;
    int polltime = -1;
    sensors_event_t *data_tmp, *data_tmp2;
    int64_t timestmp;
    int nr = 0;
    bool pollRemote;
    
    if (!remoteDevicePresent()) {
    		return 0;
    }
    
    if (!initialized) {
    	init_remote_device();
    } 

    do {
    	/* For handling disconnections */
    	if (!remoteDevicePresent()) {
    		return 0;
    	}
    	    
        int i = 0;
        
        for (i; count && i<numRSensors ; i++) {
            SensorBase* const sensor(rSensors[i]);

            if ((rPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
            	if (i == mpl_accel)
            	    data_tmp = data;
            	
                int nb = sensor->readEvents(data, count);

                if (nb < count) {
                    // no more data for this sensor
                    rPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;

                //special handling for the mpl, which has multiple handles
                if(i==mpl) {
                    i+=2; //skip accel and timer
                    rPollFds[mpl_accel].revents = 0;
                    rPollFds[mpl_timer].revents = 0;
                }
                if(i==mpl_accel) {
                    i+=1; //skip timer
                    rPollFds[mpl_timer].revents = 0;
                }
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            int i;

            do {
                
                n = poll(&rPollFds[mpl_accel], 1, nbEvents ? 0 : polltime);

            } while (n < 0 && errno == EINTR);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}
