import serial
from time import sleep

# Configuration
baud_rate = 9600
arduino_ports = {
    'engine-room' : '/dev/ttyACM0', #'/dev/serial/by-id/usb-Arduino__www.arduino.cc__Arduino_Mega_2560_93140364233351C0A1D1-if00',
    'crypt-gears' : None,
    }
arduinos = dict([(name, None) for name in arduino_ports])
received_messages = dict([(name, '') for name in arduino_ports])
received_complete_messages = dict([(name, False) for name in arduino_ports])

####################
# Define helper methods for communicating with the Arduinos
####################
def initialize_arduinos():
    for (name, port) in arduino_ports.iteritems():
        if port is None:
            continue
        # Open the port, then wait for the arduino to start (opening the port resets it).
        arduinos[name] = serial.Serial(port, baud_rate, timeout=1)
        sleep(3)
        arduinos[name].flushInput()
        # Initialize the message state.        
        received_messages[name] = ''
        received_complete_messages[name] = False

def cleanup_arduinos():
    for (name, arduino) in arduinos.iteritems():
        if arduino is None:
            continue
        arduino.close()
        
def get_message(arduino_name, inlcude_newline=False):
    process_serial()
    if received_complete_messages[arduino_name]:
        new_message = received_messages[arduino_name]
        received_complete_messages[arduino_name] = False
        received_messages[arduino_name] = ''
        return new_message if inlcude_newline else new_message.strip()
    return None
    
def process_serial():
    for arduino_name, arduino in arduinos.iteritems():
        if arduino is None:
            continue
        # If there are characters in the serial buffer,
        # and there isn't a completed message that hasn't been used yet,
        # get the new characters.
        if (not received_complete_messages[arduino_name]):
          while arduino.inWaiting() > 0:
            received_messages[arduino_name] += arduino.read()
            if received_messages[arduino_name][-1] == '\n':
              received_complete_messages[arduino_name] = True
              break

####################
# An example usage of the helper methods
####################

# Initialize.
print 'Initializing the Arduinos...'
initialize_arduinos()
print 'Arduinos initialized'

while(True):
    sleep(.01)
    for arduino_name in received_messages:
        # Get the latest message from the arduino if one has been received.
        message = get_message(arduino_name)
        if message is not None:
            # Do something with the new message.
            print '\tArduino %s says: %s' % (arduino_name, message)

# Cleanup so the serial ports don't stay busy.
cleanup_arduinos()
