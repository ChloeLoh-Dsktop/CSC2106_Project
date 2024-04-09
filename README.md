# CSC2106 IoT Project

## Dependencies
### Webserver
1. Redis
2. Docker
3. uvicorn
4. Django
5. Python3

### LoRa code
1. Arduino IDE with LMIC, AES, Adafruit, SPI and HAL libraries and Arduino UNO configured

### MQTT code
1. paho.mqtt library
2. pycryptodome library (For Windows and MacOS, change filename in python libraries from 'crypto' to "Crypto")
3. pymysql library



## Before running the codes
1. Ensure that Gateway is configured and up
2. Connect LoRa Shield nodes to sensors properly (Refer to video)
3. Don't place gateway too close to LoRa Shield nodes to minimise JOIN-ACCEPT feedback loop (About 1-3 metres is ok)
4. Ensure dependencies are properly installed

## Running the codes

### Webserver code
1. run redis instance on docker
2. run the webserver.py file on port 5000
3. use the following code to run django (cd into myproject first)
```
uvicorn myproject.asgi:application --reload 
``` 

### MQTT subscriber (Same device as Webserver)
1. Ensure topic names are correct
2. Run with python3 (In the same folder)
```
python3 mqtt.py
```

### LoRa/LoRaWAN code
1. Ensure DEVEUI, APPEUI and APPKEY matches for that node.
2. Upload the code for each node to the respective device via Arduino IDE
3. Connect nodes to an external power source.

### Connections
1. Connect IR Sensor's D0 to A0 of LORA
2. Connect Ultrasonic Sensor's Trigger and Echo Pins to A1 and A2 of LORA respectively
3. Connect LED to A3 of LORA
