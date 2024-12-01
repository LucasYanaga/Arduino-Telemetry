#include <SoftwareSerial.h>

SoftwareSerial BTSerial(8, 9); // RX | TX

void setup() {
  Serial.begin(9600);
  BTSerial.begin(38400);  // Velocidade padrão do modo AT
  Serial.println("Pronto para comandos AT");
}

void loop() {
  if (BTSerial.available()) {
    Serial.write(BTSerial.read());
  }
  if (Serial.available()) {
    BTSerial.write(Serial.read());
  }
}