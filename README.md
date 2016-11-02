# Venduino-RFID-MQTT
Connected Arduino Vending Machine with payment by RFID cards checked against an InterSystems Caché DB.

The system is composed of 3 connected parts:
- The Vending machine
- A MQTT Broker. 
- An InterSystems Ensemble Server running the Beta version 2016.4, with a Production running.

## Vending Machine

The original Venduino Vending machine by Ryan Bates (http://www.retrobuiltgames.com/diy-kits-shop/venduino/) is outfitted with aditional components:
 *  The Arduino Uno is replaced with a Arduino AT Mega 2560
 *  An Ethernet Shield is mounted on top of the AT Mega
 *  An MFRC-522 RFID Reader is mounted at the back of the coin slot. The coin slot is hidden with Company Logo Sticker
 *  An RGB Led is mounted in the LED hole on the front panel.
 *  The servos are Futaba S3003 transformed as continuous rotation.
 *  The power is supplied through the USB input, using a USB "battery pack".

The Venduino RFID-MQTT connects through an MQTT Broker to an InterSystems Ensemble server that 
  - contains the RFID Card details (CardHolder Name and Card Balance)
  - stores all transactions
  - allows to centrally manage the pricing of the Venduino-RFID-MQTT
  
Note: Before deploying this code, the Server IP Address needs to be changed
 
 ## MQTT Broker
 
The MQTT Broker used for this demo was Moquitto v1.4.10, as configured out of the box, listening on port 1883, with no security features, and running on the same server as InterSystems Ensemble.

 ## InterSystems Ensemble

The version of Ensemble is the Beta 2016.4 which has an early version of the Full Java Business Services and Business Operation. The server side code harnesses this capability, as the Input (Business Service) and Output components (Business Operation) are written un pure Java, leveraging the Apache Paho client library for MQTT.

To install and run the server side code:
- Compile the 2 Java components and generate a jar file for each.
- Load the Server classes of the Ensemble Production and Start the Production, to have the "Java Initiator" running for the next step.
- Use the ensemble portal funcionality (only available in te beta of 2016.4) to import the class contained in each Jar as a Business Service or Business Operation.
- Restart the Production.
- Initialize some values by running:
  do ##class(PYD.Venduino.Util).Init()

