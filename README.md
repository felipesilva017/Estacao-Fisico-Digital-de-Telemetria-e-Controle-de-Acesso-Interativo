# Estação Físico-Digital de Telemetria e Controle de Acesso Interativo

> **Disciplina:** Computação Física e Sistemas Embarcados (2026.2)  
> **Instituição:** Universidade Federal do Ceará (UFC) — Instituto UFC Virtual (IUVI)  
> **Curso:** Bacharelado em Sistemas e Mídias Digitais (SMD)  
> **Docente:** Prof. Clemilson C. Santos  
> **Discente:** José Felipe de Mesquita Silva (Matrícula: 538425)

---

## 📌 Visão Geral do Projeto

Este repositório contém os artefatos práticos e o código-fonte da **Parte 2 da Avaliação Integrada (Unidades 3 e 4)**. O projeto consiste em um sistema físico-digital interativo de controle de acesso e telemetria ambiental em tempo real, integrando sensoriamento analógico e digital, atuadores eletromecânicos e luminosos, e um barramento serial padronizado I2C.

O sistema comunica-se de forma **bidirecional e não-bloqueante** via enlace serial UART (115200 bauds) com uma aplicação multimídia externa desenvolvida em **p5.js** com **WebSerial API**, permitindo tanto a monitoração remota (uplink) quanto o teleacionamento de hardware (downlink).

---

## 🏗️ Estrutura do Repositório

```text
├── sketch.ino       # Firmware embarcado em C++ (Arduino UNO / Simulação no Wokwi)
├── sketch.js        # Aplicação multimídia gráfica interativa (p5.js + WebSerial API)
├── index.html       # Estrutura HTML/CSS para carregar o p5.js e os controles no navegador
└── README.md        # Documentação técnica do projeto
```

---

## 🔌 Hardware e Tabela de Conexões (Arduino UNO)

| Periférico | Pinos no Módulo | Pino no Arduino | Tipo de Sinal / Protocolo | Função no Sistema |
| :--- | :--- | :--- | :--- | :--- |
| **Display LCD 16x2 I2C** | VCC, GND, SDA, SCL | 5V, GND, **A4**, **A5** | Barramento I2C (`0x27`) | Exibição local de telemetria e modo operacional |
| **Sensor HC-SR04** | Trig, Echo | **D12**, **D11** | Digital (Tempo de Voo) | Detecção métrica de aproximação |
| **Potenciômetro** | Terminal Central | **A0** | Analógico (0 a 1023) | Controle manual filtrado via algoritmo EMA |
| **Servomotor SG90** | Sinal PWM | **D9** | PWM / PPM (50 Hz) | Cancela eletromecânica de acesso |
| **LED RGB (Cátodo)** | R, G, B | **D6**, **D5**, **D3** | Saídas Digitais / PWM | Sinalização de estados (Verde / Amarelo / Vermelho) |
| **Buzzer Piezoelétrico** | Positivo | **D8** | Digital (`tone()`) | Alarme acústico de proximidade crítica (< 15 cm) |

---

## ⚙️ Características Técnicas do Firmware (`sketch.ino`)

* **Arquitetura Concorrente Não-Bloqueante:** A temporização das tarefas de aquisição dos sensores, transmissão serial e atualização do LCD é gerenciada por contadores de tempo baseados em `millis()`, eliminando chamadas de `delay()` e prevenindo o transbordamento do buffer serial (*Serial Buffer Overflow*).
* **Filtro Passa-Baixas Exponencial (EMA):**
  $$\text{EMA}_t = \alpha \cdot X_t + (1 - \alpha) \cdot \text{EMA}_{t-1} \quad (\alpha = 0{,}15)$$
  Atenua ruídos elétricos de alta frequência na leitura analógica do potenciômetro, proporcionando suavidade no movimento do servomotor.
* **Máquina de Estados de Sinalização:**
  * **Distância > 40 cm:** LED Verde aceso (Zona Livre / Modo Seguro).
  * **Distância entre 15 cm e 40 cm:** LED Amarelo aceso (Atenção / Aproximação). Se a distância for < 30 cm, a cancela abre automaticamente (90°).
  * **Distância < 15 cm:** LED Vermelho aceso e alarme sonoro ativo no Buzzer (2000 Hz).

---

## 📡 Protocolo Serial Bidirecional (115200 Bauds)

A comunicação entre o microcontrolador e a aplicação externa opera através de mensagens ASCII delimitadas por quebra de linha (`\n`):

### 1. Canal de Telemetria / Uplink (Arduino → p5.js)
Pacote formatado transmitido periodicamente a cada 100 ms:
```text
DIST:<cm>,POT:<0-1023>,SERVO:<0-180>,MODE:<AUTO|REMOTE>\n
```
*Exemplo:* `DIST:28,POT:512,SERVO:90,MODE:AUTO`

### 2. Canal de Controle / Downlink (p5.js → Arduino)
Comandos despachados por cliques de botões ou ações na tela:
* `SERVO:<angulo>\n` — Define o ângulo do servo (0° a 180°) e força o modo manual remoto.
* `MODE:AUTO\n` — Restabelece o controle autônomo baseado nos sensores.
* `LED:<R>,<G>,<B>\n` — Comuta diretamente os canais de cor do LED RGB.

---

## 🎨 Aplicação Multimídia Externa (`sketch.js`)

Desenvolvida com o framework **p5.js**, a aplicação externa roda diretamente em navegadores modernos com suporte nativo à **WebSerial API** (Google Chrome ou Microsoft Edge):
1. **Radar Sonar de Distância:** Mapeia visualmente a proximidade do objeto em círculos concêntricos e atualiza dinamicamente sua cor (verde, amarelo e vermelho).
2. **Barra de Progresso Analógica:** Exibe graficamente o valor suavizado do potenciômetro (0 a 1023).
3. **Mostrador Angular:** Arco em tempo real indicando a abertura da cancela controlada pelo servomotor.
4. **Painel de Controle:** Botões para conexão serial USB, abertura da cancela em posições fixas (0°, 90°, 180°) e retorno ao modo automático.

---

## 🚀 Como Executar o Projeto

### 1. Executando a Simulação no Wokwi
1. Abra o [Wokwi](https://wokwi.com/) e crie um projeto com **Arduino UNO**.
2. Copie o conteúdo de `sketch.ino` para a aba de código do simulador.
3. Configure os componentes e conexões na aba `diagram.json` ou manualmente conforme a tabela de pinagem.
4. Clique no botão **Play** para iniciar a simulação.
5. Arraste o slider do sensor ultrassônico e gire o potenciômetro para verificar o funcionamento do display LCD, do LED RGB e do buzzer.

### 2. Executando a Aplicação Multimídia
1. Salve os arquivos `index.html` e `sketch.js` no mesmo diretório.
2. Abra o arquivo `index.html` no Google Chrome ou Microsoft Edge.
3. Clique em **🔌 Conectar Serial USB**, selecione a porta serial do dispositivo (ou a porta virtual emulada via *Wokwi Bridge*) e confirme a conexão.
4. Acompanhe a telemetria em tempo real e utilize os botões da interface para comandar a cancela e o LED remotamente.

---

## 📄 Licença e Uso Acadêmico

Este projeto foi desenvolvido estritamente para fins acadêmicos e avaliativos no âmbito da disciplina de Computação Física e Sistemas Embarcados do curso de Sistemas e Mídias Digitais da Universidade Federal do Ceará (UFC).
