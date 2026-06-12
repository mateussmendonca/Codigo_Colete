// --- Definição dos Pinos (Lado Esquerdo do ESP32 38-Pinos) ---
// Pinos de Entrada (Echos nos pinos exclusivos de Input)
const int PIN_ECHO_ESQ = 34;
const int PIN_ECHO_CEN = 35;
const int PIN_ECHO_DIR = 36; // Labeled as VP no ESP32

// Pinos de Saída (Triggers)
const int PIN_TRIG_ESQ = 13;
const int PIN_TRIG_CEN = 14;
const int PIN_TRIG_DIR = 27;

// Pinos dos Buzzers
const int PIN_BUZZER_ESQ = 26;
const int PIN_BUZZER_DIR = 25;

// Pinos dos Botões
const int PIN_BTN_UP = 33;
const int PIN_BTN_DOWN = 32;

// --- Variáveis de Controle de Distância ---
float distAtivacao = 120.0; 
const float DIST_MAX = 180.0;
const float DIST_MIN = 0.0;

// --- Armazenamento das Leituras ---
float distEsq = 999.0;
float distCen = 999.0;
float distDir = 999.0;

// --- Temporizadores (Não-bloqueantes com millis) ---
unsigned long tempoUltimaLeitura = 0;
const unsigned long INTERVALO_LEITURA = 100;

unsigned long tempoBipEsq = 0;
unsigned long tempoBipDir = 0;
bool estadoBuzzerEsq = false;
bool estadoBuzzerDir = false;

// --- Controle dos Botões (Debounce) ---
int estadoEstavelUp = HIGH;
int estadoEstavelDown = HIGH;
int ultimoSinalUp = HIGH;
int ultimoSinalDown = HIGH;
unsigned long tempoMudancaUp = 0;
unsigned long tempoMudancaDown = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Função para ler o sensor ultrassônico
float lerSensor(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  
  long duracao = pulseIn(pinEcho, HIGH, 12000);
  
  if (duracao == 0) {
    return 999.0; 
  }
  
  return (duracao * 0.0343) / 2.0; 
}

void setup() {
  Serial.begin(115200);
  
  // Configuração dos Triggers e Echos
  pinMode(PIN_TRIG_ESQ, OUTPUT);
  pinMode(PIN_ECHO_ESQ, INPUT);
  pinMode(PIN_TRIG_CEN, OUTPUT);
  pinMode(PIN_ECHO_CEN, INPUT);
  pinMode(PIN_TRIG_DIR, OUTPUT);
  pinMode(PIN_ECHO_DIR, INPUT);
  
  // Configuração dos Buzzers
  pinMode(PIN_BUZZER_ESQ, OUTPUT);
  pinMode(PIN_BUZZER_DIR, OUTPUT);
  
  // Configuração dos Botões com Pull-up interno
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  
  Serial.println("Colete Sensorial Inicializado (Modo 1 Lado da Protoboard)!");
  Serial.print("Distancia de Ativacao Inicial: ");
  Serial.print(distAtivacao / 100.0);
  Serial.println("m");
}

void loop() {
  unsigned long modoAtualMillis = millis();

  // --- 1. LEITURA DOS BOTÕES ---
  int leituraUp = digitalRead(PIN_BTN_UP);
  int leituraDown = digitalRead(PIN_BTN_DOWN);

  // Botão Aumentar
  if (leituraUp != ultimoSinalUp) tempoMudancaUp = modoAtualMillis;
  if ((modoAtualMillis - tempoMudancaUp) > DEBOUNCE_DELAY) {
    if (leituraUp != estadoEstavelUp) {
      estadoEstavelUp = leituraUp;
      if (estadoEstavelUp == LOW && distAtivacao < DIST_MAX) {
        distAtivacao += 30.0;
        Serial.print("Distancia aumentada: ");
        Serial.print(distAtivacao / 100.0);
        Serial.println("m");
      }
    }
  }
  ultimoSinalUp = leituraUp;

  // Botão Diminuir
  if (leituraDown != ultimoSinalDown) tempoMudancaDown = modoAtualMillis;
  if ((modoAtualMillis - tempoMudancaDown) > DEBOUNCE_DELAY) {
    if (leituraDown != estadoEstavelDown) {
      estadoEstavelDown = leituraDown;
      if (estadoEstavelDown == LOW && distAtivacao > DIST_MIN) {
        distAtivacao -= 30.0;
        Serial.print("Distancia diminuida: ");
        Serial.print(distAtivacao / 100.0);
        Serial.println("m");
      }
    }
  }
  ultimoSinalDown = leituraDown;

  // --- 2. ATUALIZAÇÃO DOS SENSORES ---
  if (modoAtualMillis - tempoUltimaLeitura >= INTERVALO_LEITURA) {
    tempoUltimaLeitura = modoAtualMillis;
    distEsq = lerSensor(PIN_TRIG_ESQ, PIN_ECHO_ESQ);
    distCen = lerSensor(PIN_TRIG_CEN, PIN_ECHO_CEN);
    distDir = lerSensor(PIN_TRIG_DIR, PIN_ECHO_DIR);
  }

  if (distAtivacao <= 0.0) {
    digitalWrite(PIN_BUZZER_ESQ, LOW);
    digitalWrite(PIN_BUZZER_DIR, LOW);
    return;
  }

  // --- 3. LÓGICA DE PRIORIDADE ---
  float menorDistEsq = min(distEsq, distCen);
  float menorDistDir = min(distDir, distCen);

  // --- 4. FREQUÊNCIAS DOS BUZZERS ---
  int intervaloEsq = 0;
  int intervaloDir = 0;

  if (menorDistEsq < (distAtivacao / 2.0)) intervaloEsq = 125;
  else if (menorDistEsq < distAtivacao) intervaloEsq = 250;

  if (menorDistDir < (distAtivacao / 2.0)) intervaloDir = 125;
  else if (menorDistDir < distAtivacao) intervaloDir = 250;

  // --- 5. EXECUÇÃO DO BIP ---
  if (intervaloEsq == 0) {
    digitalWrite(PIN_BUZZER_ESQ, LOW);
    estadoBuzzerEsq = false;
  } else {
    if (modoAtualMillis - tempoBipEsq >= intervaloEsq) {
      tempoBipEsq = modoAtualMillis;
      estadoBuzzerEsq = !estadoBuzzerEsq;
      digitalWrite(PIN_BUZZER_ESQ, estadoBuzzerEsq);
    }
  }

  if (intervaloDir == 0) {
    digitalWrite(PIN_BUZZER_DIR, LOW);
    estadoBuzzerDir = false;
  } else {
    if (modoAtualMillis - tempoBipDir >= intervaloDir) {
      tempoBipDir = modoAtualMillis;
      estadoBuzzerDir = !estadoBuzzerDir;
      digitalWrite(PIN_BUZZER_DIR, estadoBuzzerDir);
    }
  }
}