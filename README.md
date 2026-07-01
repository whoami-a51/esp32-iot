# Sistema IoT para Monitoramento Ambiental com ESP32

Projeto desenvolvido para a disciplina de **Oficina de Projetos em Engenharia Elétrica** da **UFPR**, consistindo em um sistema de monitoramento ambiental baseado em Internet das Coisas (IoT).

O sistema utiliza um **ESP32** como unidade central de processamento, integrado aos sensores **BMP180**, **DHT22** e **MQ135**, realizando a aquisição de dados ambientais e disponibilizando-os em uma interface web em tempo real, inspirada em sistemas SCADA.

## Funcionalidades

- Monitoramento de temperatura
- Monitoramento de umidade relativa do ar
- Monitoramento da pressão atmosférica
- Estimativa da qualidade do ar (MQ135)
- Servidor HTTP embarcado no ESP32
- Dashboard web responsivo com atualização automática
- API em JSON (`/data`) para integração com aplicações externas
- Comunicação via Wi-Fi

## Hardware Utilizado

- ESP32 DevKit
- Sensor BMP180
- Sensor DHT22 (AM2302)
- Sensor MQ135
- Placa de circuito impresso com reguladores de tensão de 5 V e 3,3 V
- Protoboard
- Jumpers

## Ligações

| Dispositivo | ESP32 |
|-------------|-------|
| BMP180 SDA | GPIO25 |
| BMP180 SCL | GPIO26 |
| DHT22 OUT | GPIO27 |
| MQ135 AO | GPIO34 |
| Alimentação | 5 V |
| GND | Comum |

## Interface Web

O sistema hospeda uma página HTML diretamente no ESP32, apresentando:

- Dashboard estilo SCADA
- Indicadores (gauges)
- Gráfico de tendência em tempo real
- Atualização automática a cada 2 segundos
- Visualização em qualquer navegador conectado à mesma rede

Também é disponibilizada uma API REST através da rota:

```
/data
```

retornando as medições em formato JSON.

## Tecnologias Utilizadas

- ESP32 Arduino Framework
- C++
- HTML
- CSS
- JavaScript
- WebServer.h
- WiFi.h
- Adafruit BMP085 Library
- DHT Sensor Library
- MQUnifiedsensor

## Estrutura do Projeto

```
/
├── firmware/
│   └── esp32.ino
├── imagens/
├── relatorio/
├── dashboard/
└── README.md
```

## Aplicações

- Monitoramento ambiental
- Internet das Coisas (IoT)
- Automação residencial
- Ensino de sistemas embarcados
- Estudos de comunicação Wi-Fi embarcada
- Protótipos de sistemas SCADA

## Autores

- Henry Barbosa Pires de Souza
- Piero Calabrese Galindo

Universidade Federal do Paraná (UFPR)  
Curso de Engenharia Elétrica
