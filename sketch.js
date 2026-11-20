 // =========================================================================
// PROJETO: ESTAÇÃO FÍSICO-DIGITAL DE TELEMETRIA E CONTROLE DE ACESSO
// AUTOR: JOSÉ FELIPE DE MESQUITA SILVA (MATRÍCULA: 538425)
// =========================================================================
 
let serialPort, serialWriter;
let distVal = 0, potVal = 0, servoAngle = 0, modeStr = "OFFLINE";
let serialRxBuffer = "";
 
async function connectSerial() {
  try {
    serialPort = await navigator.serial.requestPort();
    await serialPort.open({ baudRate: 115200 });
    serialWriter = serialPort.writable.getWriter();
    readSerialLoop();
  } catch (err) {
    console.error("Falha ao abrir porta serial:", err);
  }
}
 
async function readSerialLoop() {
  const textDecoder = new TextDecoderStream();
  serialPort.readable.pipeTo(textDecoder.writable);
  const reader = textDecoder.readable.getReader();
  while (true) {
    const { value, done } = await reader.read();
    if (done) break;
    if (value) {
      serialRxBuffer += value;
      let lines = serialRxBuffer.split("\n");
      serialRxBuffer = lines.pop(); // Mantem fragmento incompleto
      for (let line of lines) {
        parseTelemetryPacket(line.trim());
      }
    }
  }
}
 
function parseTelemetryPacket(line) {
  // Decodifica sintaxe: DIST:35,POT:512,SERVO:90,MODE:AUTO
  let tokens = line.split(",");
  for (let token of tokens) {
    let kv = token.split(":");
    if (kv.length === 2) {
      if (kv[0] === "DIST")  distVal = parseInt(kv[1]);
      if (kv[0] === "POT")   potVal = parseInt(kv[1]);
      if (kv[0] === "SERVO") servoAngle = parseInt(kv[1]);
      if (kv[0] === "MODE")  modeStr = kv[1];
    }
  }
}
 
async function sendCommand(cmdString) {
  if (!serialWriter) return;
  const encoder = new TextEncoder();
  await serialWriter.write(encoder.encode(cmdString + "\n"));
}
 
function setup() {
  createCanvas(720, 360);
}
 
function draw() {
  background(15, 23, 42); // Fundo escuro azul ardósia
 
  // 1. Radar Ultrassônico Reativo (HC-SR04)
  push();
  translate(180, 200);
  stroke(51, 65, 85);
  noFill();
  for (let r = 40; r <= 140; r += 35) ellipse(0, 0, r * 2, r * 2);
  stroke(59, 130, 246, 120);
  line(-150, 0, 150, 0);
  line(0, -150, 0, 150);
 
  let targetR = map(constrain(distVal, 2, 200), 2, 200, 20, 140);
  if (distVal < 15) fill(239, 68, 68);       // Vermelho: Perigo
  else if (distVal < 45) fill(245, 158, 11); // Amarelo: Alerta
  else fill(16, 185, 129);                   // Verde: Seguro
  noStroke();
  ellipse(0, -targetR, 18, 18);
  fill(255);
  textAlign(CENTER);
  textSize(14);
  text("Radar HC-SR04: " + distVal + " cm", 0, 155);
  pop();
 
  // 2. Barra Analógica do Potenciômetro (Filtro EMA)
  push();
  translate(400, 80);
  fill(255);
  textSize(14);
  text("Potenciômetro (Filtro EMA): " + potVal + " / 1023", 0, -10);
  fill(30, 41, 59);
  rect(0, 0, 260, 24, 6);
  fill(59, 130, 246);
  rect(0, 0, map(potVal, 0, 1023, 0, 260), 24, 6);
  pop();
 
  // 3. Indicador Angular do Servomotor (Cancela de Acesso)
  push();
  translate(530, 230);
  noFill();
  stroke(51, 65, 85);
  strokeWeight(8);
  arc(0, 0, 120, 120, PI, TWO_PI);
  stroke(modeStr === "REMOTE" ? "#f59e0b" : "#10b981");
  arc(0, 0, 120, 120, PI, map(servoAngle, 0, 180, PI, TWO_PI));
  noStroke();
  fill(255);
  textAlign(CENTER);
  textSize(14);
  text("Cancela Servo: " + servoAngle + "°", 0, 25);
  textSize(12);
  fill(modeStr === "REMOTE" ? "#f59e0b" : "#10b981");
  text("Modo: " + modeStr, 0, 45);
  pop();
}