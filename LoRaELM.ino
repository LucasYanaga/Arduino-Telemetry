#include <mcp_can.h>
#include <SPI.h>
#include <OBD2.h>

const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN);  // Instancia do MCP_CAN com o pino CS

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializa o MCP2515 a 500 kbps
  if (CAN.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while (1);
  }

  // Configura o MCP2515 no modo normal
  CAN.setMode(MCP_NORMAL);

  // Inicializa a comunicação OBD2
  if (!OBD2.begin()) {
    Serial.println("Failed to start OBD2 communication");
    while (1);
  }

  Serial.println("OBD2 communication started");
}

void loop() {
  // Solicita o PID do RPM (0x0C)
  if (OBD2.sendQuery(OBD_MODE_LIVE, PID_ENGINE_RPM)) {
    while (!OBD2.available()) {
      // Aguarda os dados do OBD2
    }

    // Lê os dados de RPM
    uint8_t data[2];
    OBD2.read(data, 2);

    // Calcula o RPM de acordo com a especificação OBD-II
    uint16_t rpm = (data[0] * 256 + data[1]) / 4;

    // Exibe o RPM no monitor serial
    Serial.print("RPM: ");
    Serial.println(rpm);
  } else {
    Serial.println("Failed to send OBD2 query");
  }

  delay(1000);  // Aguarda 1 segundo antes da próxima leitura
}
