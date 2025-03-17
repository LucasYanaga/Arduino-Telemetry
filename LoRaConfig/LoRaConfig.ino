#include <esp32_can.h>

#include <SoftwareSerial.h>
#include "LoRa_E220.h"

LoRa_E220 e220ttl(5, 4, 6, 2, 3);
void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

String msg = "";
uint32_t rpm = 0;
uint32_t engineLoad = 0;
uint32_t manifoldPressure = 0;
uint32_t kph = 0;
uint32_t tps = 0;
uint32_t engineTemp = 0;
bool loraStarted = false;

void setup() {
  Serial.begin(9600);      
  Serial.println("Configurando modulo LoRa...");

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

  Serial.println("LoRa OK!");

  delay(2000);

  Serial.println("Configurando Bluetooth e ELM327...");

  /*
  if(!myELM327.begin(BTSerial)) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Erro de conexão com o ELM327.");
    Serial.println("Erro de conexão com o ELM327.");
    while(1);                    // Pausa se não conseguir conectar
  }
  */

  Serial.println(">> Iniciando tranmissão de dados");

  delay(2000);
}

void loop() {
  simulateValues(100);
  printValues();
  ResponseStatus rs = e220ttl.sendFixedMessage(0, 3, 23, msg);
  
  /*
  switch (obd_state){
    case RPM:
    msg = "";
      rpm = myELM327.rpm();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(rpm) + ",";
        obd_state = ENG_LOAD;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(ENG_LOAD);
      }
      
      break;

    case ENG_LOAD:
      engineLoad = myELM327.engineLoad();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(engineLoad) + ",";
        obd_state = MANIFOLD_PRESS;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(MANIFOLD_PRESS);
      }
      
      break;

    case MANIFOLD_PRESS:
      manifoldPressure = myELM327.manifoldPressure();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(manifoldPressure) + ",";
        obd_state = KPH;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(KPH);
      }
      
      break;

    case KPH:
      kph = myELM327.kph();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(kph) + ",";
        obd_state = TPS;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(TPS);
      }
      
      break;

    case TPS:
      tps = myELM327.throttle();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(tps) + ",";
        obd_state = ENGINE_TEMP;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(ENGINE_TEMP);
      }
      
      break;

    case ENGINE_TEMP:
      engineTemp = myELM327.engineCoolantTemp();
      
      if (myELM327.nb_rx_state == ELM_SUCCESS){
        msg += String(engineTemp);
        obd_state = SEND_MESSAGE;
      }else if(myELM327.nb_rx_state != ELM_GETTING_MSG){
        setError(SEND_MESSAGE);
      }
      
      break;

    case SEND_MESSAGE:
      printValues();
      printValuesLCD();
      ResponseStatus rs = e220ttl.sendFixedMessage(0, 3, 23, msg);
      
      obd_state = RPM;

      break;
  } 
  */
}

void simulateValues(int msgdelay){
  msg = "";

  rpm = random(0, 8000);
  msg += String(rpm) + ",";
  delay(msgdelay);

  engineLoad = random(0, 100);
  msg += String(engineLoad) + ",";
  delay(msgdelay);

  manifoldPressure = random(0, 14);
  msg += String(manifoldPressure) + ",";
  delay(msgdelay);

  kph = random(0, 200);
  msg += String(kph) + ",";
  delay(msgdelay);

  tps = random(0, 100);
  msg += String(tps) + ",";
  delay(msgdelay);

  engineTemp = random(60, 200);
  msg += String(engineTemp);
  delay(msgdelay);
}

void printValues(){
  Serial.println("---------------------------------------");
  Serial.println("RPM       | = " + String(rpm));
  Serial.println("ENG_LOAD  | = " + String(engineLoad));
  Serial.println("FUEL_PRESS| = " + String(manifoldPressure));
  Serial.println("VELOC     | = " + String(kph));
  Serial.println("TPS       | = " + String(tps));
  Serial.println("ENG_TEMP  | = " + String(engineTemp));
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