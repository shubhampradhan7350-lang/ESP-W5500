#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>



#define W5500_CS   5
#define W5500_RST  4
#define SPI_SCK    18
#define SPI_MISO   19
#define SPI_MOSI   23

byte mac[] = {0xDE,0xAD,0xBE,0xEF,0xFE,0x01};                    // W5500 MAC ADDRESS

IPAddress ip(192, 168, 10, 20);                                   // STATIC IP
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 10, 1);
IPAddress dns(8, 8, 8, 8);

char packetBuff[256];
unsigned long lastPacketTime = 0;
const unsigned long COMMAND_TIMEOUT = 500;

EthernetUDP Udp;                                                 // UDP
const unsigned int UDP_PORT = 5000;

void processCommand(char*command)
{
    Serial.print("Command received: ");
    Serial.println(command);

    if (command[0] == 'F')
    {
        int speed = atoi(command + 1);

        Serial.print("FORWARD, speed = ");
        Serial.println(speed);
    }

    else if (command[0] == 'B')
    {
        int speed = atoi(command + 1);

        Serial.print("BACKWARD, speed = ");
        Serial.println(speed);
    }

    else if (command[0] == 'L')
    {
        int speed = atoi(command + 1);

        Serial.print("LEFT, speed = ");
        Serial.println(speed);
    }

    else if (command[0] == 'R')
    {
        int speed = atoi(command + 1);

        Serial.print("RIGHT, speed = ");
        Serial.println(speed);
    }

    else if (command[0] == 'S')
    {
        Serial.println("STOP");
    }

    else
    {
        Serial.println("UNKNOWN COMMAND");
    }
}

void setup()
{
    Serial.begin(115200);                                         // Start Serial Monitor

    delay(2000);

    Serial.println();
  
    Serial.println("      ESP32 + W5500 CONNECTION TEST     ");
    
    Serial.println();
    Serial.println("Resetting W5500...");                           // Reset W5500

    pinMode(W5500_RST, OUTPUT);

    digitalWrite(W5500_RST, LOW);
    delay(100);

    digitalWrite(W5500_RST, HIGH);
    delay(200);

    Serial.println("W5500 reset complete.");
    
    Serial.println();                                       // Initialize SPI
    Serial.println("Initializing SPI...");

    SPI.begin(SPI_SCK,SPI_MISO,SPI_MOSI,W5500_CS);

    Serial.println("SPI initialized.");

    Ethernet.init(W5500_CS);                                 //  Ethernet CS pin used

    Serial.print("W5500 CS pin: GPIO ");
    Serial.println(W5500_CS);

    Serial.println();                                       // Initialize Ethernet interface
    Serial.println("Initializing Ethernet interface...");

    Ethernet.begin(mac,ip,dns,gateway,subnet);

    delay(1000);
    
    Serial.println();                                     // Print Ethernet configuration

    Serial.println("   ETHERNET CONFIGURATION   ");
   

    Serial.print("IP Address    : ");
    Serial.println(Ethernet.localIP());

    Serial.print("Subnet Mask   : ");
    Serial.println(Ethernet.subnetMask());
    
    Serial.println();                                     
    Serial.println("Checking hardware...");

    EthernetHardwareStatus hardwareStatus =              // Check W5500 hardware
        Ethernet.hardwareStatus();

    if (hardwareStatus == EthernetNoHardware) {
        Serial.println();
        Serial.println("ERROR!");
        Serial.println(" Hardware NOT detected.");
    }

    else {
    Serial.println();
    Serial.println("SUCCESS!");
    Serial.println("Hardware detected.");
    }

    Serial.print("Hardware type: ");                      // Detected hardware

    if (hardwareStatus == EthernetW5500) {
        Serial.println("W5500");
    }
    else {
        Serial.println("Other Ethernet hardware");
    }   

    Serial.println();                                        // Check Ethernet link
    Serial.println("Checking Ethernet link...");

    if (Ethernet.linkStatus() == LinkON)
    {Serial.println("Ethernet link: ON");}
    else if (Ethernet.linkStatus() == LinkOFF)
    {Serial.println("Ethernet link: OFF");
    Serial.println();}
    else
    {Serial.println("Ethernet link: UNKNOWN");}


    Serial.println();                                                // Start UDP server
    Serial.print("Starting UDP server on port ");
    Serial.println(UDP_PORT);

    Udp.begin(UDP_PORT);

    Serial.println("UDP server started.");

    Serial.println();
    Serial.println("Waiting for UDP packets...");



    

}
void loop()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        Serial.println();
        Serial.println("UDP PACKET RECEIVED");
        
        int len = Udp.read(
            packetBuff,
            sizeof(packetBuff) - 1
        );
        if (len > 0)
        {
            packetBuff[len] = '\0';
        }

        
        Serial.print("Received from: ");
        Serial.println(Udp.remoteIP());

        Serial.print("Command: ");
        Serial.println(packetBuff);

        // Process command
        processCommand(packetBuff);

        // Reset watchdog timer
        lastPacketTime = millis();


    if (millis() - lastPacketTime > COMMAND_TIMEOUT)                                    // LOOP PREVENTION
    {
        Serial.println("COMMUNICATION TIMEOUT - STOP");
    }

        
    }
}


