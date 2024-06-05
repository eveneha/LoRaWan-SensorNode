# LoRaWAN Sensor Node Lab Experience N6

## Innovative Wireless Platforms for IoT

### Objectives

- Converge all previously learned skills into one lab.
- Setup a system with all elements of a LoRaWAN sensor node.
- Divide the system into subsystems for parallel development.

### Creating a Signal Path

![image](https://github.com/eveneha/LoRaWan-SensorNode/assets/93519480/30857839-56dd-40b5-875a-56c62fa70c56)
#### Signal Path Components

1. **Sensor**
2. **Signal Conditioning**
3. **Microcontroller**
4. **Transmission**
5. **Reception**
6. **Network Server**

### Converging All Lab Experiences

Create a LoRaWAN sensor node using all previous lab experiences:

- Use ESP32-S3 for BLE data collection.
- Tone inflection and volume control.

### Converge Lab Experiences

Steps to create the LoRaWAN sensor node:

1. Use a phone with nRF connect to simulate a temperature sensor.
2. Use ESP32-S3 for BLE data collection.
3. Implement serial communication to pass data between microcontrollers.
4. Use MKRWAN for LoRaWAN transmission.
5. Utilize TTN as the network server.
6. Capture data through an MQTT broker.

### Task

1. **Temperature Reading**: 
   - Receive a temperature reading from a BLE sensor (simulated using your phone and nRF connect) using the ESP32.
2. **Data Communication**: 
   - Communicate the temperature data between the ESP32 and the MKRWAN module using a serial protocol of your choosing.
3. **LoRaWAN Uplink**:
   - Send the data to TTN through a LoRaWAN uplink using the MKRWAN module.
4. **Data Subscription**:
   - Subscribe to TTN’s MQTT server to retrieve the data.

### Report Deadline

- **Deadline**: June 17th, 2024
- **Submission**: Submit to Portale della Didattica in the Elaborati/Homework section.
