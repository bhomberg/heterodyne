import serial
from time import sleep
from time import time
import simpleaudio as sa

## CONFIGURATION ##
baud_rate = 9600
arduino_ports = {
    'engine-room' : None, #'/dev/ttyACM1', #'/dev/serial/by-id/usb-Arduino__www.arduino.cc__Arduino_Mega_2560_93140364233351C0A1D1-if00',
    'crypt-gears' : None,
    }
arduinos = dict([(name, None) for name in arduino_ports])
received_messages = dict([(name, '') for name in arduino_ports])
received_complete_messages = dict([(name, False) for name in arduino_ports])

####################
# Define helper methods for communicating with the Arduinos
####################
def initialize_arduinos():
    for (name, port) in arduino_ports.items():
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

# TODO(bhomberg): clean this up (& process serial)
def get_message(arduino_name, inlcude_newline=False):
    process_serial(arduino_name)
    if received_complete_messages[arduino_name]:
        new_message = received_messages[arduino_name]
        received_complete_messages[arduino_name] = False
        received_messages[arduino_name] = ''
        return new_message if inlcude_newline else new_message.strip()
    return None

def process_serial(arduino_name_in):
    for arduino_name, arduino in arduinos.items():
        if arduino is None:
            continue
        if not arduino_name_in == arduino_name:
            continue
        # If there are characters in the serial buffer,
        # and there isn't a completed message that hasn't been used yet,
        # get the new characters.
        done = False
        while (not done):
            if (not received_complete_messages[arduino_name]):
                r = arduino.read()
                r = r.decode('utf_8')
                if r == '\n' or r == '\r':
                    done = True
                    if len(received_messages[arduino_name]) > 0:
                        received_complete_messages[arduino_name] = True
                    continue
                received_messages[arduino_name] += r


## INITIALIZE ##
print('Initializing the Arduinos...')
initialize_arduinos()
print('Arduinos initialized')


## VARIABLES ##
# variables for timekeeping
max_time = 20*60 # 20 min * 60 s / min
timekeeping = [False, False, False, False]
timeval = [16*60+25, 9*60+12, 4*60+44, 2*60+8]
timesound = ["sound_bites/16-25.wav", "sound_bites/9-12.wav", "sound_bites/4-44.wav", "sound_bites/2-08.wav"]

# variables for lights puzzle
lightsound = ["sound_bites/picky.wav", "sound_bites/turnoff.wav", "sound_bites/hurteyes.wav", "sound_bites/doyoueven.wav", "sound_bites/justdont.wav", "sound_bites/notdyinginthedark.wav"]

# variables for gear puzzle
gearsound = ["sound_bites/sendinghelp.wav", "sound_bites/goodbyefriends.wav"]

# variables for operator sounds
# todo -- limit switches for doompart2 (moving room locks) and
# enigmaticmechanism (unlock moving room)
operatorsound = ["sound_bites/won_tsolveanything.wav", "sound_bites/ticklish.wav", "sound_bites/donttouchnotpuzzle.wav", "sound_bites/doompart2.wav", "sound_bites/enigmaticmechanism.wav"]

# intro/outro sounds
intro = "sound_bites/foolishcreaturesblastdoorsselfdestruct.wav"
outro = "sound_bites/selfdestructcomplete.wav"


# operator loop
# runs full room lengths -- if you need to start in the middle, the script
# should be restarted instead
while(True):
  start = input("Press any key to start.")
  done = False
  doom2 = False
  enigma = False

  # play intro
  s = time()
  w = sa.WaveObject.from_wave_file(intro)
  p = w.play()
  p.wait_done()
  p = None

  while(time() - s < max_time and !done):
    sleep(.01)

    # say puzzle messages if relevant
    for arduino_name in received_messages:
      # Get the latest message from the arduino if one has been received.
        message = get_message(arduino_name)
        if message is not None:
          # Do something with the new message.
            print('\tArduino %s says: %s' % (arduino_name, message))

            # lights puzzle
            if message == '10':
              w = sa.WaveObject.from_wave_file(lightsound[0])
              p = w.play()
            if message == '11':
              w = sa.WaveObject.from_wave_file(lightsound[1])
              p = w.play()
            if message == '12':
              w = sa.WaveObject.from_wave_file(lightsound[2])
              p = w.play()
            if message == '13':
              w = sa.WaveObject.from_wave_file(lightsound[3])
              p = w.play()
            if message == '14':
              w = sa.WaveObject.from_wave_file(lightsound[4])
              p = w.play()
            if message == '15':
              w = sa.WaveObject.from_wave_file(lightsound[5])
              p = w.play()
              p.wait_done()
              p = None
            if message == '19':
              if (p != None):
                p.stop()
                p = None

            # gear puzzle
            if message == '20':
              print("ERROR!  PUZZLE 4 is BROKEN!  ABORT!")
              w = sa.WaveObject.from_wave_file(gearsound[0])
              p = w.play()
              p.wait_done()
              p = None
            if message == '21':
              done = True
              w = sa.WaveObject.from_wave_file(gearsound[1])
              p = w.play()
              p.wait_done()
              p = None

            # user input buttons / room buttons
            if message == '0':
              w = sa.WaveObject.from_wave_file(operatorsound[0])
              p = w.play()
              p.wait_done()
              p = None
            if message == '1':
              w = sa.WaveObject.from_wave_file(operatorsound[1])
              p = w.play()
              p.wait_done()
              p = None
            if message == '2':
              w = sa.WaveObject.from_wave_file(operatorsound[2])
              p = w.play()
              p.wait_done()
              p = None
            if message == '3' and !doom2:
              w = sa.WaveObject.from_wave_file(operatorsound[3])
              p = w.play()
              p.wait_done()
              p = None
              doom2 = True
            if message == '4' and !enigma:
              w = sa.WaveObject.from_wave_file(operatorsound[4])
              p = w.play()
              p.wait_done()
              p = None
              enigma = True

    # say time warnings
    for i in range(4):
        if timekeeping[i] is False and time() - s > max_time - timeval[i]:
          timekeeping[i] = True
          w = sa.WaveObject.from_wave_file(timesound[i])
          print(timesound[i])
          p = w.play()
          p.wait_done()
          p = None

  # if time is out, but has not won
  if !done:
    w = sa.WaveObject.from_wave_file(outro)
    p = w.play()
    p.wait_done()
    p = None
