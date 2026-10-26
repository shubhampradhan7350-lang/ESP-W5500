#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define W5500_CS 5
#define W5500_RST 4
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

#define ENCODER_A 34                //ENCD reading pins
#define ENCODER_B 35

int pulsecount;
int PPR=12532;
float lasttime=0;
int rpm=0;

int Leftfront[] = {12, 13}; //{dir,pwm}
int Rightfront[] = {25, 26};
int Leftback[] = {32, 33};
int Rightback[] = {22, 15};

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01}; // W5500 MAC ADDRESS

IPAddress ip(192, 168, 10, 20); // STATIC IP
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 10, 1);
IPAddress dns(8, 8, 8, 8);



char packetBuff[256];
unsigned long lastPacketTime = 0;
const unsigned long COMMAND_TIMEOUT = 3600;

EthernetUDP Udp; // UDP
const unsigned int UDP_PORT = 5000;

void linear(int dir, int pwm)
{

    digitalWrite(Leftfront[0], dir); // Motor Motion Control
    digitalWrite(Rightfront[0], dir);
    digitalWrite(Leftback[0], dir);
    digitalWrite(Rightback[0], dir);

    analogWrite(Leftfront[1], pwm);
    analogWrite(Rightfront[1], pwm);
    analogWrite(Leftback[1], pwm);
    analogWrite(Rightback[1], pwm);

    Serial.print("DIR signal: ");
    Serial.println(dir == HIGH ? "HIGH" : "LOW");
}

void processCommand(char *command) // While passing an array
{
    Serial.print("Command received: ");
    Serial.println(command);

    if (command[0] == 'F')
    {
        int speed = atoi(command + 1); // atoi is a pre defined function in the library that converts the strings to actual digit no
        linear(1, speed);              // like buff=[1,2,0] then atoi(buff) will give 120

        Serial.print("FORWARD, speed = ");
        Serial.println(speed);
    }

    else if (command[0] == 'B')
    {
        int speed = atoi(command + 1);
        linear(0, speed);

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


void  doEncoderA() {                                           //Functions to count the pulses from the encoder
  if (digitalRead(ENCODER_A) == digitalRead(ENCODER_B)) {
    pulsecount--;
  } else {
    pulsecount++;
  }
}

void  doEncoderB() {
  if (digitalRead(ENCODER_A) == digitalRead(ENCODER_B)) {
    pulsecount++;
  } else {
    pulsecount--;
  }
}



void setup()
{

    Serial.begin(115200); // Start Serial Monitor

    pinMode(Leftfront[0], OUTPUT);
    pinMode(Rightfront[0], OUTPUT);
    pinMode(Leftback[0], OUTPUT);
    pinMode(Rightback[0], OUTPUT);

    pinMode(Leftfront[1], OUTPUT);
    pinMode(Rightfront[1], OUTPUT);
    pinMode(Leftback[1], OUTPUT);
    pinMode(Rightback[1], OUTPUT);
    pinMode(ENCODER_A, INPUT);
    pinMode(ENCODER_B, INPUT);

  
    delay(1000);

    // Motor Direction Control

    Serial.println();

    Serial.println("      ESP32 + W5500 CONNECTION TEST     ");

    Serial.println();
    Serial.println("Resetting W5500..."); // Reset W5500

    pinMode(W5500_RST, OUTPUT);

    digitalWrite(W5500_RST, LOW);
    delay(100);

    digitalWrite(W5500_RST, HIGH);
    delay(200);

    Serial.println("W5500 reset complete.");

    Serial.println(); // Initialize SPI
    Serial.println("Initializing SPI...");

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, W5500_CS);

    Serial.println("SPI initialized.");

    Ethernet.init(W5500_CS); //  Ethernet CS pin used

    Serial.print("W5500 CS pin: GPIO ");
    Serial.println(W5500_CS);

    Serial.println(); // Initialize Ethernet interface
    Serial.println("Initializing Ethernet interface...");

    Ethernet.begin(mac, ip, dns, gateway, subnet);

    delay(1000);

    Serial.println(); // Print Ethernet configuration

    Serial.println("   ETHERNET CONFIGURATION   ");

    Serial.print("IP Address    : ");
    Serial.println(Ethernet.localIP());

    Serial.print("Subnet Mask   : ");
    Serial.println(Ethernet.subnetMask());

    Serial.println();
    Serial.println("Checking hardware...");

    EthernetHardwareStatus hardwareStatus = // Check W5500 hardware
        Ethernet.hardwareStatus();

    if (hardwareStatus == EthernetNoHardware)
    {
        Serial.println();
        Serial.println("ERROR!");
        Serial.println(" Hardware NOT detected.");
    }

    else
    {
        Serial.println();
        Serial.println("SUCCESS!");
        Serial.println("Hardware detected.");
    }

    Serial.print("Hardware type: "); // Detected hardware

    if (hardwareStatus == EthernetW5500)
    {
        Serial.println("W5500");
    }
    else
    {
        Serial.println("Other Ethernet hardware");
    }

    Serial.println(); // Check Ethernet link
    Serial.println("Checking Ethernet link...");

    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.println("Ethernet link: ON");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
        Serial.println("Ethernet link: OFF");
        Serial.println();
    }
    else
    {
        Serial.println("Ethernet link: UNKNOWN");
    }

    Serial.println(); // Start UDP server
    Serial.print("Starting UDP server on port ");
    Serial.println(UDP_PORT);

    Udp.begin(UDP_PORT);

    Serial.println("UDP server started.");

    Serial.println();
    Serial.println("Waiting for UDP packets...");

                                                                         // Attach interrupts for EVERY CHANGE on both channels (4x decoding)
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), doEncoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), doEncoderB, CHANGE);
}
void loop()
{

    int packetSize = Udp.parsePacket();
    Serial.println(packetSize);
    if (packetSize)
    {
        lastPacketTime = millis();
        Serial.println();
        Serial.println("UDP PACKET RECEIVED");

        int len = Udp.read(
            packetBuff,
            sizeof(packetBuff) - 1);
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
        delay(600);
    }
    else
    {
        if (millis() - lastPacketTime > COMMAND_TIMEOUT)
        {
            Serial.println();
            Serial.println("No command received for 3000ms. Motor stopped.");
            linear(0, 0);
            delay(600);
        }
        else
        {
            processCommand(packetBuff);
            delay(600);
        }
    }

    if(millis() - lasttime >= 1000)
    {
       float dt=(millis() - lasttime)/1000.0;

       rpm=(pulsecount/(PPR*dt))*(60/dt);
       lasttime=millis();
       pulsecount=0;

       Serial.print("  Actual RPM: ");
       Serial.println(rpm);
    }
}

// Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
//& "C:\Users\SHUBHAM\OneDrive\Desktop\DTU CLG\INFERNO\rover_control.ps1"