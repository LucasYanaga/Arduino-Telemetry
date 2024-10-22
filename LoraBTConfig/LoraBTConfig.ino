#include <SoftwareSerial.h>

SoftwareSerial mySerial(8, 9); // 8 - RXArudino -> TXBT, 9 - TXArduino -> RXBT

void setup() {
  // Inicializa as portas seriais
  Serial.begin(9600);  // Serial monitor (para comunicação com o PC)
  mySerial.begin(38400);  // Serial do HC-05 (para comunicação com o módulo)

  // Entrar no modo AT manualmente
  pinMode(7, OUTPUT); // Pino 9 para controlar o EN (Enable) do HC-05
  digitalWrite(7, HIGH); // Colocar o pino EN em HIGH para modo AT

  delay(2000);
  
  // Iniciar configuração
  Serial.println("Iniciando configuração do HC-05...");

  btStatus();
  btConfig();
}

void loop() {

}

void btStatus(){
  sendATcommand("AT");
  sendATcommand("AT+VERSION");
  sendATcommand("AT+PSWD");
  sendATcommand("AT+ADDR?");
  sendATcommand("AT+ADDE");
  sendATcommand("AT+MRAD"); // Most Recent Device Connection
  sendATcommand("AT+STATE?");
}

void btConfig(){
  sendATcommand("AT+RESET");
  sendATcommand("AT+ORGL");
  sendATcommand("AT+ROLE=1");
  sendATcommand("AT+CMODE=0");
  sendATcommand("AT+BIND=0011,22,334455");
  sendATcommand("AT+INIT");
  sendATcommand("AT+PAIR=0011,22,334455,20");
  sendATcommand("AT+LINK=0011,22,334455");
  sendATcommand("AT+STATE?");
}

void sendATcommand(String command){
  Serial.println(command);
  mySerial.println(command);
  delay(100);

  while(mySerial.available()){
    delay(5);

    if(mySerial.available() > 0){
        char c = mySerial.read();
        Serial.print(c);
    }
  }

  Serial.println("------------------------");
  delay(2000);

  /*
    // Leitura do PC para o HC-05
    if (Serial.available()) {
      mySerial.write(Serial.read());
    }

    // Leitura do HC-05 para o PC
    if (mySerial.available()) {
      Serial.write(mySerial.read());
    }

    delay(2000);
    */
}
