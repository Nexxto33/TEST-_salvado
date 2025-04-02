import RPi.GPIO as GPIO

if GPIO.input(button1) == GPIO.LOW:
            ser.write("A\n".encode('utf-8'))
            enviar_temp = False
            time.sleep(0.1)
            print("Boton")
            enviar_temp = True