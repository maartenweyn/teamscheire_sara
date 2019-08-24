#!/usr/bin/env python

# Maarten Weyn
# maarten@wesdec.be

import serial
import time



ranges = [-1, -1, -1, -1, -1, -1]
counter = [-1, -1, -1, -1, -1, -1]
last_id = 100

port = serial.Serial("/dev/tty.usbserial-FT98HUH7", baudrate=500000, timeout=3.0)
print (port.is_open)

try:
    while True:
        line = port.readline() 
        if (not line.startswith( "[ERROR]" )):
            #print("- ", data.strip())
            data = line.strip().split(',')
            # print (data, len(data))

            if (len(data) == 3):
                try: 
                    id = int(data[0]) - 1
                    # print (data[0], id)
                    range = int(data[2])
                    # print (data[2], range)
                except ValueError:
                    print(error, data)
                    continue

                ranges[id] = range
                counter[id] = 0
                for i in [x for x in xrange(6) if x != id]:
                    if counter[i] < 100:
                        counter[i] += 1
        
                if (id < last_id):
                    print(ranges, counter)

                last_id = id


        # 1,271791869574,2646
        # 2,547293288678,3256

except (KeyboardInterrupt, SystemExit):
    port.close()
    print("interrrupted - closing")
except Exception as e:
    print("exception - closing: ", str(e))
