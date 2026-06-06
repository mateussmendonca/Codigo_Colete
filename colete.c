// --- Definição dos Pinos ---
const int PIN_TRIG_ESQ = 12;
const int PIN_ECHO_ESQ = 13;
const int PIN_TRIG_CEN = 14;
const int PIN_ECHO_CEN = 27;
const int PIN_TRIG_DIR = 26;
const int PIN_ECHO_DIR = 25;

const int PIN_BUZZER_ESQ = 32;
const int PIN_BUZZER_DIR = 33;

const int PIN_BTN_UP = 18;
const int PIN_BTN_DOWN = 19;

// --- Variáveis de Controle de Distância ---
float distAtivacao = 120.0; // Distância inicial padrão: 1,20m (em centímetros)
const float DIST_MAX = 180.0;
const float DIST_MIN = 0.0;

// --- Armazenamento das Leituras ---
float distEsq = 999.0;
float distCen = 999.0;
float distDir = 999.0;

// --- Temporizadores (Não-bloqueantes com millis) ---
unsigned long tempoUltimaLeitura = 0;
const unsigned long INTERVALO_LEITURA = 100; // Lê os sensores a cada 100ms

unsigned long tempoBipEsq = 0;
unsigned long tempoBipDir = 0;
bool estadoBuzzerEsq = false;
bool estadoBuzzerDir = false;

float ultimaDistGer = 999.0;
unsigned long tempoInativ = 0;
const unsigned long TEMPO_MUDO = 15000; 
const float TOLERANCIA = 10.0;    
bool modoMudo = false;

// --- Controle dos Botões (Debounce Antirruído Corrigido) ---
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
  
  // Timeout de 12000 microssegundos limita a leitura a cerca de 2 metros
  long duracao = pulseIn(pinEcho, HIGH, 12000); 
  
  if (duracao == 0) {
    return 999.0; // Se falhar ou estiver fora do alcance, retorna um valor alto
  }
  
  return (duracao * 0.0343) / 2.0; // Conversão para centímetros
}

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos dos sensores
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
  
  Serial.println("Colete Sensorial Inicializado!");
  Serial.print("Distancia de Ativacao Inicial: ");
  Serial.print(distAtivacao / 100.0);
  Serial.println("m");
}

void loop() {
  unsigned long modoAtualMillis = millis();

  // --- 1. LEITURA DOS BOTÕES (INTERFACE COM DEBOUNCE FILTRADO) ---
  int leituraUp = digitalRead(PIN_BTN_UP);
  int leituraDown = digitalRead(PIN_BTN_DOWN);

  // Tratamento do Botão Aumentar (UP)
  if (leituraUp != ultimoSinalUp) {
    tempoMudancaUp = modoAtualMillis; // Reseta o cronômetro se o pino oscilar (ruído)
  }
  if ((modoAtualMillis - tempoMudancaUp) > DEBOUNCE_DELAY) {
    if (leituraUp != estadoEstavelUp) {
      estadoEstavelUp = leituraUp; // O sinal se estabilizou no novo estado
      
      // Só executa a ação se o estado estável for LOW (botão pressionado)
      if (estadoEstavelUp == LOW) {
        if (distAtivacao < DIST_MAX) {
          distAtivacao += 30.0;
          Serial.print("Distancia aumentada para: ");
          Serial.print(distAtivacao / 100.0);
          Serial.println("m");
        }
      }
    }
  }
  ultimoSinalUp = leituraUp;

  // Tratamento do Botão Diminuir (DOWN)
  if (leituraDown != ultimoSinalDown) {
    tempoMudancaDown = modoAtualMillis; // Reseta o cronômetro se o pino oscilar (ruído)
  }
  if ((modoAtualMillis - tempoMudancaDown) > DEBOUNCE_DELAY) {
    if (leituraDown != estadoEstavelDown) {
      estadoEstavelDown = leituraDown; // O sinal se estabilizou no novo estado
      
      // Só executa a ação se o estado estável for LOW (botão pressionado)
      if (estadoEstavelDown == LOW) {
        if (distAtivacao > DIST_MIN) {
          distAtivacao -= 30.0;
          Serial.print("Distancia diminuida para: ");
          Serial.print(distAtivacao / 100.0);
          Serial.println("m");
        }
      }
    }
  }
  ultimoSinalDown = leituraDown;


  // --- 2. ATUALIZAÇÃO DOS SENSORES (A CADA 100MS) ---
  if (modoAtualMillis - tempoUltimaLeitura >= INTERVALO_LEITURA) {
    tempoUltimaLeitura = modoAtualMillis;
    
    distEsq = lerSensor(PIN_TRIG_ESQ, PIN_ECHO_ESQ);
    distCen = lerSensor(PIN_TRIG_CEN, PIN_ECHO_CEN);
    distDir = lerSensor(PIN_TRIG_DIR, PIN_ECHO_DIR);
  }

  // Se a distância de ativação for 0, silencia tudo e pula o resto da lógica
  if (distAtivacao <= 0.0) {
    digitalWrite(PIN_BUZZER_ESQ, LOW);
    digitalWrite(PIN_BUZZER_DIR, LOW);
    return; 
  }

  // --- 3. LOGICA DE PRIORIDADE (MAIS PRÓXIMO) ---
  float menorDistEsq = min(distEsq, distCen);
  float menorDistDir = min(distDir, distCen);
  
  float menorDistGer = min(menorDistEsq, menorDistDir);
  if (abs(menorDistGer - ultimaDistGer) <= TOLERANCIA){
      if (modoAtualMillis - tempoInativ >= TEMPO_MUDO){
        modoMudo = true;
      }
  }
else {
    tempoInativ = modoAtualMillis; 
    ultimaDistGer = menorDistGer;   
    modoMudo = false; 
  }
  // --- 4. CONTROLE DAS FREQUÊNCIAS DOS BUZZERS ---
  int intervaloEsq = 0; 
  int intervaloDir = 0;
if(!modoMudo){
  // Determina velocidade do Buzzer Esquerdo
  if (menorDistEsq < (distAtivacao / 2.0)) {
    intervaloEsq = 125; // 4 vezes por segundo
  } else if (menorDistEsq < distAtivacao) {
    intervaloEsq = 250; // 2 vezes por segundo
  }

  // Determina velocidade do Buzzer Direito
  if (menorDistDir < (distAtivacao / 2.0)) {
    intervaloDir = 125; // 4 vezes por segundo
  } else if (menorDistDir < distAtivacao) {
    intervaloDir = 250; // 2 vezes por segundo
  }
}
  // --- 5. EXECUÇÃO DO BIP DOS BUZZERS (SEM BLOQUEIO) ---
  // Buzzer Esquerdo
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

  // Buzzer Direito
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
