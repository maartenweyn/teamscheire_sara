import serial
import time

#port = serial.Serial("/dev/tty.SLAB_USBtoUART", baudrate=500000, timeout=3.0)
port = serial.Serial("/dev/tty.usbserial-FT98HUH7", baudrate=500000, timeout=3.0)
print (port.is_open)
while True:
  size = port.inWaiting()
  if size:
      data = port.read(size)
      print (data)
  else:
      print ('no data ', size)
  time.sleep(1)