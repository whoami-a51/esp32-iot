/*
 * ESP32 - Dashboard Web
 * Sensores: DHT22, BMP180, MQ135
 * Henry Barbosa e Piero Calabrese
 * 
 *   - Scanner I2C para diagnóstico
 *   - Tratamento de R0 = inf no MQ135
 *   - Classes de qualidade do ar
 *   - Fallback seguro para sensores
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <MQUnifiedsensor.h>

// ============= CONFIGURAÇÕES DE REDE =============
const char* ssid     = "";
const char* password = "";

IPAddress local_IP(192, 168, 0, 50);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// ============= PINAGEM DOS SENSORES =============
#define DHT22_PIN  27
#define DHT_TYPE   DHT22
#define BMP_SDA    25
#define BMP_SCL    26
#define MQ135_PIN  34

// ============= INICIALIZAÇÃO DOS SENSORES =============
DHT dht(DHT22_PIN, DHT_TYPE);
Adafruit_BMP085 bmp;

// Configuração MQUnifiedsensor
#define Board          ("ESP-32")
#define Voltage_Resolution (3.3)
#define ADC_Bit_Resolution (12)
#define RatioMQ135CleanAir (3.6)
MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ135_PIN, "MQ-135");

WebServer server(80);

// ============= VARIÁVEIS =============
float temperatura = NAN;
float umidade     = NAN;
float pressao     = NAN;
float altitude    = NAN;
float qualidadeAr = NAN;
float rZero       = NAN;

unsigned long lastRead = 0;
const unsigned long interval = 2000;

bool bmpOk = false;

// ============= CLASSIFICAÇÃO DA QUALIDADE DO AR =============
String classificarQualidade(float ppm) {
  if (isnan(ppm) || ppm <= 0) return "Sem dado";
  if (ppm < 400)  return "Excelente";
  if (ppm < 600)  return "Boa";
  if (ppm < 1000) return "Moderada";
  if (ppm < 1500) return "Alerta";
  if (ppm < 2000) return "Crítica";
  return "Perigosa";
}

// ============= LEITURA DOS SENSORES =============
void lerSensores() {
  // --- DHT22 ---
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temperatura = t;
    umidade = h;
  }

  // --- BMP180 ---
  if (bmpOk) {
    pressao  = bmp.readPressure() / 100.0F;
    altitude = bmp.readAltitude(1013.25);
  } else {
    pressao  = 925.0;   // valor fixo de fallback
    altitude = NAN;     // altitude continua sem dado
  }

  // --- MQ135 ---
  MQ135.setA(110);
  MQ135.setB(-0.63);
  MQ135.update();
  
  float leitura = MQ135.readSensor();
  if (!isinf(leitura) && !isnan(leitura) && leitura > 0) {
    qualidadeAr = leitura;
  }
  rZero = MQ135.getR0();
}

// ============= PÁGINA HTML =============
String gerarPagina() {
  String pagina = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<title>SCADA Industrial</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body {
  margin: 0;
  font-family: Arial;
  background: #0b1220;
  color: #d1d5db;
}
.header {
  background: #0f172a;
  padding: 12px;
  text-align: center;
  border-bottom: 2px solid #1f2937;
}
.header h1 {
  margin: 0;
  color: #38bdf8;
  font-size: 18px;
}
.grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
  padding: 15px;
}
.card {
  background: #111827;
  border: 1px solid #1f2937;
  border-radius: 10px;
  padding: 12px;
  text-align: center;
}
.label {
  font-size: 12px;
  color: #9ca3af;
}
.value {
  font-size: 26px;
  font-weight: bold;
  margin-top: 8px;
}
.ok { color: #22c55e; }
.warn { color: #facc15; }
.bad { color: #ef4444; }
.gaugeBox {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
  padding: 15px;
}
.gaugeCard {
  background: #111827;
  border: 1px solid #1f2937;
  border-radius: 10px;
  padding: 12px;
  text-align: center;
}
.gauge {
  width: 140px;
  height: 70px;
  margin: 10px auto;
  border-radius: 140px 140px 0 0;
  background: #1f2937;
  position: relative;
  overflow: hidden;
}
.fill {
  position: absolute;
  bottom: 0;
  width: 100%;
  height: 50%;
  transition: 0.4s;
}
.chartBox {
  margin: 15px;
  background: #111827;
  border: 1px solid #1f2937;
  padding: 10px;
  border-radius: 10px;
}
.footer {
  background: #0f172a;
  padding: 10px;
  font-size: 12px;
  border-top: 1px solid #1f2937;
  display: flex;
  justify-content: space-between;
}
</style>
</head>
<body>
<div class="header">
  <h1>SISTEMA SCADA - MONITORAMENTO DE CONDIÇÕES FÍSICAS</h1>
</div>
<div class="grid">
  <div class="card">
    <div class="label">TEMPERATURA</div>
    <div id="temp" class="value">--</div>
  </div>
  <div class="card">
    <div class="label">UMIDADE</div>
    <div id="hum" class="value">--</div>
  </div>
  <div class="card">
    <div class="label">PRESSÃO</div>
    <div id="press" class="value">--</div>
  </div>
  <div class="card">
    <div class="label">QUALIDADE DO AR</div>
    <div id="gasText" class="value ok">BOA</div>
  </div>
</div>
<div class="gaugeBox">
  <div class="gaugeCard">
    <div class="label">TEMPERATURA</div>
    <div class="gauge"><div id="gTemp" class="fill" style="height:50%;background:#ef4444;"></div></div>
    <div id="tVal" class="value">--</div>
  </div>
  <div class="gaugeCard">
    <div class="label">UMIDADE</div>
    <div class="gauge"><div id="gHum" class="fill" style="height:50%;background:#22c55e;"></div></div>
    <div id="hVal" class="value">--</div>
  </div>
  <div class="gaugeCard">
    <div class="label">PRESSÃO</div>
    <div class="gauge"><div id="gPress" class="fill" style="height:50%;background:#3b82f6;"></div></div>
    <div id="pVal" class="value">--</div>
  </div>
  <div class="gaugeCard">
    <div class="label">QUALIDADE DO AR</div>
    <div class="gauge"><div id="gGas" class="fill" style="height:50%;background:#22c55e;"></div></div>
    <div id="gVal" class="value">--</div>
  </div>
</div>
<div class="chartBox">
  <div class="label">TENDÊNCIA DO SISTEMA</div>
  <canvas id="chart"></canvas>
</div>
<div class="footer">
  <div>STATUS: <span style="color:#22c55e">ONLINE</span></div>
  <div>Desenvolvido por Henry Barbosa e Piero Calabrese</div>
</div>
<script>
function gasStatus(v) {
  if (v < 400)  return { txt: "EXCELENTE", color: "#22c55e" };
  if (v < 600)  return { txt: "BOA",       color: "#76ff03" };
  if (v < 1000) return { txt: "MODERADA",  color: "#facc15" };
  if (v < 1500) return { txt: "ALERTA",    color: "#ff9800" };
  if (v < 2000) return { txt: "CRÍTICA",   color: "#ef4444" };
  return { txt: "PERIGOSA", color: "#d50000" };
}
const ctx = document.getElementById("chart");
const chart = new Chart(ctx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      { label: "Temperatura", data: [], borderColor: "red",   yAxisID: "yTemp",  tension: 0.3 },
      { label: "Umidade",     data: [], borderColor: "cyan",  yAxisID: "yHum",   tension: 0.3 },
      { label: "Pressão",     data: [], borderColor: "lime",  yAxisID: "yPress", tension: 0.3 },
      { label: "Qualidade do Ar",       data: [], borderColor: "orange",yAxisID: "yGas",   tension: 0.3 }
    ]
  },
  options: {
    animation: false,
    plugins: { legend: { labels: { color: "#d1d5db" } } },
    scales: {
      x: { ticks: { color: "#9ca3af" } },
      yTemp:  { type: "linear", position: "left",  min: 10, max: 40,   ticks: { stepSize: 5,  color: "#9ca3af" } },
      yHum:   { type: "linear", position: "right", min: 0,  max: 100,  ticks: { stepSize: 20, color: "#9ca3af" }, grid: { drawOnChartArea: false } },
      yPress: { type: "linear", position: "right", min: 900, max: 1025, ticks: { stepSize: 25, color: "#9ca3af" }, grid: { drawOnChartArea: false } },
      yGas:   { type: "linear", position: "left",  min: 0,  max: 3000, ticks: { stepSize: 500, color: "#9ca3af" }, grid: { drawOnChartArea: false } }
    }
  }
});
async function fetchData() {
  try {
    const resp = await fetch("/data");
    const d = await resp.json();
    const t = new Date().toLocaleTimeString();
    const temp = parseFloat(d.temperatura) || 0;
    const hum  = parseFloat(d.umidade) || 0;
    const press = parseFloat(d.pressao) || 925;
    const gas  = parseFloat(d.qualidadeAr) || 0;
    const st = gasStatus(gas);
    document.getElementById("temp").innerText = temp.toFixed(1) + " °C";
    document.getElementById("hum").innerText  = hum.toFixed(1) + " %";
    document.getElementById("press").innerText = press.toFixed(1) + " hPa";
    const ge = document.getElementById("gasText");
    ge.innerText = st.txt;
    ge.style.color = st.color;
    let tPct = Math.min(100, Math.max(0, ((temp - 10) / 30) * 100));
    document.getElementById("gTemp").style.height = tPct + "%";
    document.getElementById("gTemp").style.background = "#ef4444";
    document.getElementById("tVal").innerText = temp.toFixed(1) + "°C";
    let hPct = Math.min(100, Math.max(0, hum));
    document.getElementById("gHum").style.height = hPct + "%";
    document.getElementById("gHum").style.background = "#22c55e";
    document.getElementById("hVal").innerText = hum.toFixed(1) + "%";
    let pPct = Math.min(100, Math.max(0, ((press - 900) / 125) * 100));
    document.getElementById("gPress").style.height = pPct + "%";
    document.getElementById("gPress").style.background = "#3b82f6";
    document.getElementById("pVal").innerText = press.toFixed(1);
    let gPct = Math.min(100, (gas / 3000) * 100);
    document.getElementById("gGas").style.height = gPct + "%";
    document.getElementById("gGas").style.background = st.color;
    document.getElementById("gVal").innerText = Math.floor(gas);
    chart.data.labels.push(t);
    chart.data.datasets[0].data.push(temp);
    chart.data.datasets[1].data.push(hum);
    chart.data.datasets[2].data.push(press);
    chart.data.datasets[3].data.push(gas);
    if (chart.data.labels.length > 30) {
      chart.data.labels.shift();
      chart.data.datasets.forEach(ds => ds.data.shift());
    }
    chart.update();
  } catch(e) {
    console.error("Erro", e);
  }
}
setInterval(fetchData, 2000);
fetchData();
</script>
</body>
</html>
)rawliteral";

  return pagina;
}
// ============= ROTAS =============
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", gerarPagina());
}

void handleData() {
  String json = "{";
  json += "\"temperatura\":" + String(isnan(temperatura) ? 0.0 : temperatura, 1) + ",";
  json += "\"umidade\":" + String(isnan(umidade) ? 0.0 : umidade, 1) + ",";
  json += "\"pressao\":" + String(isnan(pressao) ? 0.0 : pressao, 1) + ",";
  json += "\"altitude\":" + String(isnan(altitude) ? 0.0 : altitude, 1) + ",";
  json += "\"qualidadeAr\":" + String(isnan(qualidadeAr) ? 0 : qualidadeAr, 0) + ",";
  json += "\"classificacao\":\"" + classificarQualidade(qualidadeAr) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "404 - Pagina nao encontrada");
}

// ============= SETUP =============
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Iniciando Estacao Ambiental ESP32 ===");

  // I2C para BMP180
  Wire.begin(BMP_SDA, BMP_SCL);

  // Scanner I2C para diagnóstico
  Serial.println("Escaneando I2C...");
  byte count = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo I2C encontrado em 0x");
      Serial.println(addr, HEX);
      count++;
    }
    delay(5);
  }
  if (count == 0) Serial.println("Nenhum dispositivo I2C encontrado!");

  // DHT22
  dht.begin();

  // BMP180
  if (bmp.begin()) {
    Serial.println("BMP180 OK.");
    bmpOk = true;
  } else {
    Serial.println("BMP180 NAO ENCONTRADO!");
    bmpOk = false;
  }

  // MQ135 - Configuração
  MQ135.setRegressionMethod(1);
  MQ135.init();
  MQ135.setR0(10.0);

  MQ135.setA(110.0);
  MQ135.setB(-0.63);

  Serial.print("Calibrando MQ135. Aguarde...");
  float calcR0 = 0;
  int amostrasValidas = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    float leitura = MQ135.calibrate(RatioMQ135CleanAir);
    if (!isinf(leitura) && !isnan(leitura) && leitura > 0) {
      calcR0 += leitura;
      amostrasValidas++;
    }
    Serial.print(".");
    delay(500);
  }

  if (amostrasValidas > 0) {
    MQ135.setR0(calcR0 / amostrasValidas);
    Serial.println("\nMQ135 calibrado! R0 = " + String(MQ135.getR0()));
  } else {
    MQ135.setR0(10.0);
    Serial.println("\nFalha na calibracao MQ135 (R0=inf), usando R0=10.0 como fallback");
  }

  // WiFi - IP fixo
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Falha ao configurar IP fixo!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi...");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.print("IP fixo: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha WiFi. Reiniciando...");
    ESP.restart();
  }

  // Servidor
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor rodando na porta 80");
}

// ============= LOOP =============
void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastRead >= interval) {
    lastRead = now;
    lerSensores();

    Serial.print("Temp: ");
    if (isnan(temperatura)) Serial.print("--");
    else { Serial.print(temperatura, 1); }
    Serial.print("°C | Umi: ");
    if (isnan(umidade)) Serial.print("--");
    else { Serial.print(umidade, 1); }
    Serial.print("% | Press: ");
    if (isnan(pressao)) Serial.print("--");
    else { Serial.print(pressao, 1); }
    Serial.print("hPa | CO2: ");
    if (isnan(qualidadeAr)) Serial.print("--");
    else { Serial.print(qualidadeAr, 0); }
    Serial.print("ppm | ");
    Serial.println(classificarQualidade(qualidadeAr));
  }
}
