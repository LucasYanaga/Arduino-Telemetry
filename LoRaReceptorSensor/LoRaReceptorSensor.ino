#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <Servo.h>
#include <ArduinoJson.h>

Servo servo;
 
#define LED1 8 
#define INA 9
#define LED_VERDE 12
#define LED_AMARL 11
#define LED_VERML 10

LoRa_E220 e220ttl(5, 3, 6, 2, 3); // Arduino RX <-- e220 TX, Arduino TX --> e220 RX AUX M0 M1
void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

int servoCount = 0;

// Cria um buffer de memória para armazenar o JSON lido
StaticJsonDocument<200> json;

//JSON
String led = "";
int motor = 0;
String sonic = "";

String lastLedState = "off";

int* valoresOBD;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(INA, OUTPUT);
  analogWrite(INA, 255);

  Serial.begin(9600);

  delay(500);

  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(LED_VERDE, HIGH);

// Startup all pins and UART
  e220ttl.begin();

  ResponseStructContainer c;
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  Configuration configuration = *(Configuration*) c.data;
  Serial.println("Configuração Atual: ");
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  //  ----------------------- DEFAULT TRANSPARENT WITH RSSI -----------------------
  configuration.ADDL = 0x03;
  configuration.ADDH = 0x00;

  configuration.CHAN = 23;
  
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.airDataRate = AIR_DATA_RATE_110_384;
  configuration.SPED.uartParity = MODE_00_8N1;
  
  configuration.OPTION.subPacketSetting = SPS_200_00;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_ENABLED;
  configuration.OPTION.transmissionPower = POWER_22;
  
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_500_000;

  configuration.CRYPT.CRYPT_H = 0x01;
	configuration.CRYPT.CRYPT_L = 0x01;

  Serial.println("Setando Configuração: ");
  ResponseStatus rs = e220ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  Serial.println(rs.getResponseDescription());
  Serial.println(rs.code);
 
  Serial.println("Configuração Após Setar: ");
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);
 
  printParameters(configuration);
 
  c.close();

  Serial.println(">> Iniciando recepção de dados");

}
 
void loop() { 
	// If something available
  if (e220ttl.available()>1) {
	  ResponseContainer rc = e220ttl.receiveMessageRSSI();

    // Is something goes wrong print error
    if (rc.status.code!=1){
      Serial.println(rc.status.getResponseDescription());
    }else{
      // Print the data received
      //Serial.println("Recebendo: " + rc.data);

      int rssiDbm = toDbm(rc.rssi);
      String input = rc.data;

      if (input.substring(0, 1) != "{") {
        split(input, rssiDbm);
      }else{
        DeserializationError error = deserializeJson(json, input);

        if(json["id"] == 2){
          led = json["led"].as<String>();
          motor = json["motor"].as<int>();
          sonic = json["sonic"].as<String>();

          printValues(led, motor, sonic, rssiDbm);

          if(led == "on" && led != lastLedState) {
            lastLedState = led;
            digitalWrite(LED1, HIGH);  
          } 
          if(led == "off" && led != lastLedState) {
            lastLedState = led;
            digitalWrite(LED1, LOW);
          }

          if(motor >= 0){
            runMotor(motor);
          }

          if(sonic.toInt() != 0L){
            runSonicLed(sonic.toInt());
          }
      }
    }
    }
  }
}

void runMotor(int speed){
  analogWrite(INA, speed);
}

void runSonicLed(int value){
  if(value <= 10){
    turnLedOn(3);
  }else if(value > 10 && value <= 30){
    turnLedOn(2);
  }else{
    turnLedOn(1);
  }
}

void turnLedOn(int led){
  analogWrite(LED_VERDE, 0);
  analogWrite(LED_AMARL, 0);
  analogWrite(LED_VERML, 0);

  if(led == 1){
    analogWrite(LED_VERDE, 255);
  }

  if(led == 2){
    analogWrite(LED_AMARL, 255);
  }

  if(led == 3){
    analogWrite(LED_VERML, 255);
  }
}

int toDbm(int rssi){
  return -(256 - rssi);
}

String getSignalQuality(int rssiDbm){

    if(rssiDbm >= -30){
      return "EXCELENTE";
    }else if(rssiDbm < -30 && rssiDbm >= -50){
      return "MUITO BOM";
    }else if( rssiDbm < -50 && rssiDbm >= -70){
      return "BOM";
    }else if( rssiDbm < -70 && rssiDbm >= -90){
      return "NORMAL";
    }else if( rssiDbm < -90 && rssiDbm >= -100){
      return "RUIM";
    }else if( rssiDbm < - 100){
      return "MUITO RUIM";
    }else{
      return "SEM CONEXÃO";
    }
}

void split(String str, int rssiDbm){
  int StringCount = 0;
  String value = "";
  while (str.length() > 0){
    int index = str.indexOf(',');
    if (index == -1) {// No space found{
      value = str;
      break;
    }
    else{
      value = str.substring(0, index);
      str = str.substring(index+1);
    }
    StringCount++;
    switch(StringCount - 1){
      case 0:
        Serial.print("RPM   | = ");
        break;
      case 1:
        Serial.print("ENG_L | = ");
        break;
      case 2:
        Serial.print("COL_P | = ");
        break;
      case 3:
        Serial.print("VEL   | = ");
        break;
      case 4:
        Serial.print("TPS   | = ");
        break;
      case 5:
        Serial.print("ENG_T | = ");
        break;      
    }
    Serial.println(value);
  }
  Serial.print("Sinal: "); Serial.println(getSignalQuality(rssiDbm) + " | " + rssiDbm + "dBm");
  Serial.println("---------------------------------------");
}

void printValues(String led, int motor, String sonic, int rssiDbm){
  Serial.println("LED   | = " + led);
  Serial.println("MOTOR | = " + String(motor));
  Serial.println("SONIC | = " + sonic);
  Serial.print("Sinal: "); Serial.println(getSignalQuality(rssiDbm) + " | " + rssiDbm + "dBm");
  Serial.println("---------------------------------------");
}

void printValuesOBD(int rssiDbm){
  for (int i = 0; i < 7; i++) {
    Serial.println(valoresOBD[i]);
  }
  /*
  Serial.println("---------------------------------------");
  Serial.println("RPM       | = " + String(valoresOBD[0]));
  Serial.println("ENG_LOAD  | = " + String(valoresOBD[1]));
  Serial.println("FUEL_PRESS| = " + String(valoresOBD[2]));
  Serial.println("VELOC     | = " + String(valoresOBD[3]));
  Serial.println("TPS       | = " + String(valoresOBD[4]));
  Serial.println("ENG_TEMP  | = " + String(valoresOBD[5]));*/
  Serial.print("Sinal: "); Serial.println(getSignalQuality(rssiDbm) + " | " + rssiDbm + "dBm");
  Serial.println("---------------------------------------");
}

void printParameters(struct Configuration configuration) {
    Serial.println("----------------------------------------");
 
    Serial.print(F("HEAD : "));  Serial.print(configuration.COMMAND, HEX);Serial.print(" ");Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(configuration.LENGHT, HEX);
    Serial.println(F(" "));
    Serial.print(F("AddH : "));  Serial.println(configuration.ADDH, HEX);
    Serial.print(F("AddL : "));  Serial.println(configuration.ADDL, HEX);
    Serial.println(F(" "));
    Serial.print(F("Chan : "));  Serial.print(configuration.CHAN, DEC); Serial.print(" -> "); Serial.println(configuration.getChannelDescription());
    Serial.println(F(" "));
    Serial.print(F("SpeedParityBit     : "));  Serial.print(configuration.SPED.uartParity, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTParityDescription());
    Serial.print(F("SpeedUARTDatte     : "));  Serial.print(configuration.SPED.uartBaudRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTBaudRateDescription());
    Serial.print(F("SpeedAirDataRate   : "));  Serial.print(configuration.SPED.airDataRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getAirDataRateDescription());
    Serial.println(F(" "));
    Serial.print(F("OptionSubPacketSett: "));  Serial.print(configuration.OPTION.subPacketSetting, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getSubPacketSetting());
    Serial.print(F("OptionTranPower    : "));  Serial.print(configuration.OPTION.transmissionPower, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getTransmissionPowerDescription());
    Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
    Serial.println(F(" "));
    Serial.print(F("TransModeWORPeriod : "));  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
    Serial.print(F("TransModeEnableLBT : "));  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
    Serial.print(F("TransModeEnableRSSI: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
    Serial.print(F("TransModeFixedTrans: "));  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());
 
 
    Serial.println("----------------------------------------");
}
void printModuleInformation(struct ModuleInformation moduleInformation) {
    Serial.println("----------------------------------------");
    Serial.print(F("HEAD: "));  Serial.print(moduleInformation.COMMAND, HEX);Serial.print(" ");Serial.print(moduleInformation.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(moduleInformation.LENGHT, DEC);
 
    Serial.print(F("Model no.: "));  Serial.println(moduleInformation.model, HEX);
    Serial.print(F("Version  : "));  Serial.println(moduleInformation.version, HEX);
    Serial.print(F("Features : "));  Serial.println(moduleInformation.features, HEX);
    Serial.println("----------------------------------------");
}