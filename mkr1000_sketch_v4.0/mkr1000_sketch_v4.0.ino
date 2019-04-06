/*
  Title  : Arduino MKR1000
  version: V4.
  Contact: info@tatco.cc
  Done By: TATCO Inc.
  github : https://github.com/rabee2050/arduino-mkr1000
  ios    :
  Android:

  Release Notes:
  - V1 Created 1 Jan 2018
  - V2 Skipped
  - V3 Skipped
  - V4 Updated 10 Oct 2018

*/

#include <SPI.h>
#include <WiFi101.h>
#include <Servo.h>

char ssid[] = "Mirabee";        // your network SSID (name)
char pass[] = "1231231234";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);

#define lcdSize 3 //this will define number of LCD on the phone app
String protectionPassword = ""; //This will not allow anyone to add or control your board.
String boardType;

char pinsMode[54];
int pinsValue[54];
Servo servoArray[53];

String lcd[lcdSize];
unsigned long serialTimer = millis();
byte digitalArraySize, analogArraySize;

String httpAppJsonOk = "HTTP/1.1 200 OK \n content-type:application/json \n\n";
String httpTextPlainOk = "HTTP/1.1 200 OK \n content-type:text/plain \n\n";

void setup(void)
{
  Serial.begin(115200);      // initialize serial communication

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  server.begin();                           // start the web server on port 80
  printWiFiStatus();                        // you're connected now, so print out the status
  boardInit();                              // Init the board
}

void loop(void)
{
  lcd[0] = "Test 1 LCD";// you can send any data to your mobile phone.
  lcd[1] = "Test 2 LCD";// you can send any data to your mobile phone.
  lcd[2] = String(analogRead(1));//  send analog value of A1

  WiFiClient client = server.available();
  if (client) {                             // if you get a client,
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        process( client);
        break;
      }
    }
  }
  update_input();
  printWifiSerial();
}

void process(WiFiClient client) {
  String getString = client.readStringUntil('/');
  String arduinoString = client.readStringUntil('/');
  String command = client.readStringUntil('/');

  if (command == "digital") {
    digitalCommand(client);
  }

  if (command == "pwm") {
    pwmCommand(client);
  }

  if (command == "servo") {
    servoCommand(client);
  }

  if (command == "terminal") {
    terminalCommand(client);
  }

  if (command == "mode") {
    modeCommand(client);
  }

  if (command == "allonoff") {
    allonoff(client);
  }

  if (command == "password") {
    changePassword(client);
  }

  if (command == "allstatus") {
    allstatus(client);
  }

}
void terminalCommand(WiFiClient client) {//Here you recieve data form app terminal
  String data = client.readStringUntil('/');
  client.print(httpAppJsonOk + "Ok from Arduino " + String(random(1, 100)));
  delay(1); client.stop();
  
  Serial.println(data);
}

void digitalCommand(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
    delay(1); client.stop();
  }
}

void pwmCommand(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    analogWrite(pin, value);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
    delay(1); client.stop();
  }
}

void servoCommand(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    servoArray[pin].write(value);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
    delay(1); client.stop();
  }
}

void modeCommand(WiFiClient client) {
  String  pinString = client.readStringUntil('/');
  int pin = pinString.toInt();
  String mode = client.readStringUntil('/');
  if (mode != "servo") {
    servoArray[pin].detach();
  };

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    pinsMode[pin] = 'o';
    pinsValue[pin] = 0;
    allstatus(client);
  }
  if (mode == "push") {
    pinsMode[pin] = 'm';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus(client);
  }
  if (mode == "schedule") {
    pinsMode[pin] = 'c';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus(client);
  }

  if (mode == "input") {
    pinsMode[pin] = 'i';
    pinsValue[pin] = 0;
    pinMode(pin, INPUT);
    allstatus(client);
  }

  if (mode == "pwm") {
    pinsMode[pin] = 'p';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
    allstatus(client);
  }

  if (mode == "servo") {
    pinsMode[pin] = 's';
    pinsValue[pin] = 0;
    servoArray[pin].attach(pin);
    servoArray[pin].write(0);
    allstatus(client);
  }

}

void allonoff(WiFiClient client) {
  int pin, value;
  value = client.parseInt();
  for (byte i = 0; i <= 16; i++) {
    if (pinsMode[i] == 'o') {
      digitalWrite(i, value);
      pinsValue[i] = value;
    }
  }
  client.print(httpTextPlainOk + value);
  delay(1); client.stop();
}

void changePassword(WiFiClient client) {
  String data = client.readStringUntil('/');
  protectionPassword = data;
  client.print(httpAppJsonOk);
  delay(1); client.stop();
}


void allstatus(WiFiClient client) {
  String dataResponse;
  dataResponse += F("HTTP/1.1 200 OK \n");
  dataResponse += F("content-type:application/json \n\n");
  dataResponse += "{";
  dataResponse += "\"m\":[";//m for mode
  for (byte i = 0; i <= digitalArraySize; i++) {
    dataResponse += "\"";
    dataResponse += pinsMode[i];
    dataResponse += "\"";
    if (i != digitalArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"v\":[";//v for value
  for (byte i = 0; i <= digitalArraySize; i++) {
    dataResponse += pinsValue[i];
    if (i != digitalArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"a\":[";//a for analog value
  for (byte i = 0; i <= analogArraySize; i++) {
    dataResponse += analogRead(i);
    if (i != analogArraySize)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"l\":[";//l for LCD value
  for (byte i = 0; i <= lcdSize - 1; i++) {
    dataResponse += "\"";
    dataResponse += lcd[i];
    dataResponse += "\"";
    if (i != lcdSize - 1)dataResponse += ",";
  }
  dataResponse += "],";
  dataResponse += "\"t\":\""; //t for Board Type .
  dataResponse += boardType;
  dataResponse += "\",";
  dataResponse += "\"p\":\""; // p for Password.
  dataResponse += protectionPassword;
  dataResponse += "\"";
  dataResponse += "}";
  client.print(dataResponse);
  delay(1); client.stop();
}

void update_input() {
  for (byte i = 0; i < sizeof(pinsMode); i++) {
    if (pinsMode[i] == 'i') {
      pinsValue[i] = digitalRead(i);
    }
  }
}

void boardInit() {
  for (byte i = 0; i <= 14; i++) {
    pinsMode[i] = 'o';
    pinsValue[i] = 0;
    pinMode(i, OUTPUT);
  }
  boardType = "mkr1000";
  digitalArraySize = 14;
  analogArraySize = 6;
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

}

void printWifiSerial() {
  if (Serial.read() > 0) {
    if (millis() - serialTimer > 3000) {
      printWiFiStatus();
    }
    serialTimer = millis();
  }
}
