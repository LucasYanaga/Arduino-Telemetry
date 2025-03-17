import serial
from tabulate import tabulate
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import csv
import keyboard
from datetime import datetime

# Configuração da tabela
headers = ['CAR', 'RPM', 'ENG_L', 'COL_P', 'VEL', 'TPS', 'ENG_T', 'CONN', 'QUALITY']
car_name = "MIT_Lancer"

# Função para classificar o RSSI
def classify_rssi(rssi_dbm):
    if rssi_dbm >= -30:
        return "EXCELENTE", "green"
    elif -50 <= rssi_dbm < -30:
        return "MUITO BOM", "lime"
    elif -70 <= rssi_dbm < -50:
        return "BOM", "yellowgreen"
    elif -90 <= rssi_dbm < -70:
        return "NORMAL", "orange"
    elif -100 <= rssi_dbm < -90:
        return "RUIM", "red"
    else:
        return "MUITO RUIM", "darkred"

# Configuração da porta serial
port = "COM5"  # Porta serial do Arduino
baud = 9600    # Baud rate do Arduino
ser = serial.Serial(port, baud)
print(f"Connected to Arduino at Port: {port} and Baud: {baud}")

# Dados para os gráficos
time_data = []
variables = {  # Cada variável será associada a um gráfico
    "RPM": [],
    "ENG_L": [],
    "COL_P": [],
    "VEL": [],
    "TPS": [],
    "ENG_T": [],
}

# Configuração do painel de gráficos (2x3)
fig, axes = plt.subplots(2, 3, figsize=(18, 10))  # Ajustando para janela maior (1920x1080)
fig.suptitle(f"{car_name} Dashboard", fontsize=16)
axes = axes.flatten()  # Transforma a matriz de eixos em uma lista

# Intervalo de animação para atualização dos gráficos
update_interval = 500  # Em ms, aumentando o intervalo de atualização para reduzir a frequência

# Limitação de pontos para exibição
display_interval = 10  # Exibe os últimos 10 pontos

recording = False
csv_file = None
csv_writer = None
recording_text = None

# Inicializa os gráficos
lines = {}
for ax, var in zip(axes, variables.keys()):
    ax.set_title(var)
    ax.set_xlabel("Time (s)")
    ax.set_ylabel(var)
    ax.grid()
    line, = ax.plot([], [], label=var)
    lines[var] = line

# Adiciona o indicador de gravação no painel
indicator_ax = fig.add_axes([0.8, 0.01, 0.2, 0.03])  # Coordenadas: [esquerda, inferior, largura, altura]
indicator_ax.axis("off")
recording_text = indicator_ax.text(0.5, 0.5, "Gravação: Parada", 
                                    horizontalalignment='center', 
                                    verticalalignment='center', 
                                    fontsize=12, 
                                    color="red")

# Adiciona o campo de classificação da conexão e valor de sinal
conn_indicator_ax = fig.add_axes([0.6, 0.01, 0.2, 0.03])  # Coordenadas: [esquerda, inferior, largura, altura]
conn_indicator_ax.axis("off")
conn_text = conn_indicator_ax.text(0.5, 0.5, "Conexão: SEM CONEXÃO - SEM SINAL", 
                                   horizontalalignment='center', 
                                   verticalalignment='center', 
                                   fontsize=12, 
                                   color="blue")

# Atualiza os gráficos em tempo real
def update(frame):
    global time_data, variables, recording, csv_file, csv_writer

    get_data = ser.readline()
    data_string = get_data.decode('utf-8', errors='ignore')

    if data_string[:4] == "data":
        splited_data = data_string[5:].split(",")
        
        rssi_dbm = int(splited_data[6])  # valor do sinal RSSI
        connection_quality = classify_rssi(rssi_dbm)  # Classificação do sinal
        conn_value = f"{rssi_dbm} dBm - {connection_quality[0]}"  # Exibindo valor e classificação

        # Atualiza o eixo do tempo
        if len(time_data) == 0:
            time_data.append(0)
        else:
            time_data.append(time_data[-1] + 1)

        # Atualiza os valores de cada variável
        variables["RPM"].append(float(splited_data[0]))
        variables["ENG_L"].append(float(splited_data[1]))
        variables["COL_P"].append(float(splited_data[2]))
        variables["VEL"].append(float(splited_data[3]))
        variables["TPS"].append(float(splited_data[4]))
        variables["ENG_T"].append(float(splited_data[5]))

        # Limita os dados para exibição (limita para 10 pontos)
        if len(time_data) > display_interval:
            time_data = time_data[-display_interval:]
            for key in variables:
                variables[key] = variables[key][-display_interval:]

        # Atualiza o status da conexão e valor de sinal no painel
        conn_text.set_text(f"Conexão: {conn_value}")
        conn_text.set_color(connection_quality[1])

        # Atualiza todas as linhas com novos dados
        for var, line in lines.items():
            line.set_data(time_data, variables[var])

        # Ajusta os limites dos gráficos
        for ax, var in zip(axes, variables.keys()):
            ax.set_xlim(max(0, time_data[-1] - display_interval), time_data[-1] + 10)
            ax.set_ylim(min(variables[var]) - 10, max(variables[var]) + 10)

        # Grava os dados no arquivo CSV se estiver gravando
        if recording and csv_writer:
            # Remover quebras de linha no campo de conexão
            clean_conn_value = conn_value.replace("\n", "").replace("\r", "")
            csv_writer.writerow([car_name] + [splited_data[0], splited_data[1], splited_data[2], splited_data[3], splited_data[4], splited_data[5], splited_data[6], connection_quality[0]])

    return list(lines.values())  # Retorna todas as linhas para atualizar os gráficos

# Função para alternar a gravação ao pressionar Enter
def toggle_recording():
    global recording, csv_file, csv_writer, recording_text

    if not recording:
        # Iniciar gravação com nome do arquivo dinâmico
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        file_name = f"{car_name}_{timestamp}.csv"
        csv_file = open(file_name, 'w', newline='', encoding='utf-8')
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(headers)
        recording = True
        recording_text.set_text("Gravação: Em andamento")
        recording_text.set_color("green")
    else:
        # Parar gravação
        recording = False
        csv_file.close()
        recording_text.set_text("Gravação: Parada")
        recording_text.set_color("red")

# Detecta pressionamento de tecla Enter para alternar gravação
keyboard.add_hotkey('enter', toggle_recording)

# Animação
ani = FuncAnimation(fig, update, interval=update_interval)

# Ajusta o layout para tela 1920x1080
fig.set_size_inches(18, 10)
plt.tight_layout(rect=[0, 0.03, 1, 0.95])  # Ajusta para dar espaço ao título principal

# Exibe os gráficos
plt.show()
