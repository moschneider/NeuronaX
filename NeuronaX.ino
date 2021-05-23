#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <CuteBuzzerSounds.h>
#include <ShiftRegister74HC595.h>

/**
 * 
 * REDE NEURAL ARTIFICIAL (PERCEPTRON COM 3 CAMADAS)
 * -------------------------------------------------
 * 
 * A = numero de entradas da rede
 * B = numero de neuronios escondidos
 * C = numero de saidas
 * 
 * x[i] = entradas (i = 0,..,A-1)
 * h[j] = neuronios escondidos (j = 0,..,B-1)
 * o[k] = saida (calculada) da rede (k = 0,..,C-1)
 * y[k] = saida desejada da rede (k = 0,..,C-1)
 * 
 * w[i][j] = sinapses entre a camada de entrada e a camada escondida (i = 0,..,A-1; j = 0,..,B-1)
 * q[j][k] = sinapses entre a camada escondida e camada de saida (j = 0,..,B-1; k = 0,..,C-1)
 * 
 * 
 * BACKPROPAGATION
 * ---------------
 * 
 * u[k] = erros nas saidas (k = 0,..,C-1)
 * s[j] = somas temporarias na camada escondida (j = 0,..,B-1)
 * f[j] = erros na camada escondida (j = 0,..,B-1)
 * 
 * deltaQ[j][k] = diferenca calculada nas sinapses entre camada escondida e camada de saida (j = 0,..,B-1; k = 0,..,C-1)
 * deltaW[i][j] = diferenca calculada nas sinapses entre camada de entrada e camada escondida (i = 0,..,A-1; j = 0,..,B-1)
 *
 * N = taxa de aprendizado (ex. 0,75)
 * 
*/

/* Rede */
  
float x[3], h[3], o[3], y[3], w[3][3], q[3][3];

const byte A = 3, B = 3, C = 3;

/* Backpropagation */

float u[3], s[3], f[3], deltaQ[3][3], deltaW[3][3];

float N = 0.75;

/* Pares de entrada e saida pre programados */

byte X[7][3], Y[7][3];

/* Repeticoes de demonstracao */

const byte  rep          = 3;

/* Botoes e seus valores */

const byte  button1Pin   = 2;
const byte  button2Pin   = 4;
const byte  button3Pin   = A1;

byte button1Val         = 0;
byte button2Val         = 0;
byte button3Val         = 0; 

/* Joystick e seus valores */

const byte  joyXPin      = A2;
const byte  joyYPin      = A3;
const byte  joyButtonPin = 7;

int  joyXVal            = 0;
int  joyYVal            = 0;
byte joyButtonVal       = 0;

const byte  buzzerPin    = 8;

/* Dois shift registers em daisy chain  */
// parameters: <number of shift registers> (data pin, clock pin, latch pin)
ShiftRegister74HC595<2> sr(12, A0, 13);

/* Pins de elementos da rede (portas PWM) */

const byte  h1Pin              = 3;
const byte  h2Pin              = 5;
const byte  h3Pin              = 6;

const byte  o1Pin              = 9;
const byte  o2Pin              = 10;
const byte  o3Pin              = 11;

/* Constantes para direcionamento das explicacoes */

const byte  ff           = 0;
const byte  bp           = 1;
const byte  el           = 2;

/* Direcoes de joystick */

const byte UP                  = 0;
const byte DOWN                = 1;
const byte LEFT                = 2;
const byte RIGHT               = 3;
const byte NONE                = 4;

/* Som de tick ligado/desligado */

boolean silent = false;

/* Display LCD - ATENCAO: Encontrar o endereco de seu display com o I2C_Scanner */

LiquidCrystal_I2C lcd(0x27,20,4);

/* Inicializacao de pins e interface serial (para debug) */
/* Titulo e instrucoes de navegacao */
/* Inicializacao da rede e do conjunto de treinamento */

void setup() {

  Serial.begin(9600);

  pinMode(button1Pin,INPUT_PULLUP);
  pinMode(button2Pin,INPUT_PULLUP);
  pinMode(button3Pin,INPUT_PULLUP);

  pinMode(joyXPin,INPUT);
  pinMode(joyYPin,INPUT);

  pinMode(joyButtonPin,INPUT_PULLUP);

  pinMode(h1Pin, OUTPUT);
  pinMode(h2Pin, OUTPUT);
  pinMode(h3Pin, OUTPUT);

  pinMode(o1Pin, OUTPUT);
  pinMode(o2Pin, OUTPUT);
  pinMode(o3Pin, OUTPUT);

  pinMode(buzzerPin, OUTPUT);
  cute.init(buzzerPin);
  
  lcd.init();
  lcd.backlight();

  cPrint(0,F("[ NEURONA X ]"));
  cPrint(1,F("=Simulador de MLP="));
  cPrint(2,F("desenvolvido 2021"));
  cPrint(3,F("por M. Schneider"));

  allOn();

  cute.play(S_CONNECTION);

  delay(3000);

  lcd.clear();

  cPrint(0,F("* ATENCAO *"));
  cPrint(1,F("Navegar nos menus"));
  cPrint(2,F("com Joystick"));
  cPrint(3,F("[Exit] Continuar"));

  cute.play(S_CONNECTION);

  allOff();

  while(exitButtonPressed())
    readInterface();

  while(exitButtonPressed()==false)
    readInterface();

  delay(500);

  randomSeed(analogRead(0));

  initRede();

  initPares();
}

/* Rapido "tick" no buzzer */

void tick()
{
  if(silent==false)
  {
    digitalWrite(buzzerPin,HIGH);
    delay(20);
    digitalWrite(buzzerPin,LOW);
    delay(30);
  }
}

/* Imprimir na tela de maneira centralizada */

void cPrint(int line, String text)
{
  lcd.setCursor(10-text.length()/2,line);
  lcd.print(text);
}

/* Ligar todos os leds */

void allOn()
{
  digitalWrite(h1Pin,HIGH);
  digitalWrite(h2Pin,HIGH);
  digitalWrite(h3Pin,HIGH);

  digitalWrite(o1Pin,HIGH);
  digitalWrite(o2Pin,HIGH);
  digitalWrite(o3Pin,HIGH);

  sr.setAllHigh();
}

/* Desligar todos os leds */

void allOff()
{
  digitalWrite(h1Pin,LOW);
  digitalWrite(h2Pin,LOW);
  digitalWrite(h3Pin,LOW);

  digitalWrite(o1Pin,LOW);
  digitalWrite(o2Pin,LOW);
  digitalWrite(o3Pin,LOW);

  sr.setAllLow();
}

/* Ler status de botoes e joystick */

void readInterface()
{
  button1Val         = digitalRead(button1Pin);
  button2Val         = digitalRead(button2Pin);
  button3Val         = digitalRead(button3Pin);
  joyXVal            = analogRead(joyXPin);
  joyYVal            = analogRead(joyYPin);
  joyButtonVal       = digitalRead(joyButtonPin);  
}

/* Retorna se joystick ou botao de exit foram acionados */

boolean joyMoved()
{
  if(joyXVal<100) return true;
  if(joyXVal>900) return true;
  if(joyYVal<100) return true;
  if(joyYVal>900) return true;
  if(joyButtonVal==0) return true;
  if(button3Val==0) return true;

  return false;
}

/* Retorna direcao do joystick */

byte getJoyDir()
{
  int bX = analogRead(joyXPin);
  int bY = analogRead(joyYPin);

  if(bX<200) return UP;
  if(bX>800) return DOWN;
  if(bY<200) return LEFT;
  if(bY>800) return RIGHT;

  return NONE;
}

/* Valores na tela para teste de hardware */

void dumpInterfaceValues()
{
  cPrint(0,F("[ Testar Hardware ]"));
  lcd.setCursor(0,2);
  lcd.print("btn");
  lcd.setCursor(4,2);
  lcd.print(F("joystick"));
  lcd.setCursor(16,2);
  lcd.print(F("exit"));
  lcd.setCursor(0,3);
  lcd.print(F("                    "));
  lcd.setCursor(0,3);
  lcd.print(button1Val);
  lcd.setCursor(2,3);
  lcd.print(button2Val);
  lcd.setCursor(4,3);
  lcd.print(joyXVal);
  lcd.setCursor(9,3);
  lcd.print(joyYVal);
  lcd.setCursor(14,3);
  lcd.print(joyButtonVal);
  lcd.setCursor(16,3);
  lcd.print(button3Val);
}

/* Blink de um elemento ligado nos shift registers */

void blkSr(byte pn, int dly)
{
  for(byte i=0;i<rep;i++)
  {
    sr.set(pn, HIGH);
    tick();
    delay(dly);
    sr.set(pn, LOW);
    tick();
    delay(dly);
  }
}

/* Blink de um elemento em pin */

void blkPn(byte pn, int dly)
{
  for(byte i=0;i<rep;i++)
  {
    digitalWrite(pn, HIGH);
    tick();
    delay(dly);
    digitalWrite(pn, LOW);
    tick();
    delay(dly);
  }
}

/* Ligar um elemento em pin */

void onPn(byte pn, int dly)
{
  digitalWrite(pn, HIGH);
  delay(dly);  
}

/* Desligar um elemento em pin */

void offPn(byte pn, int dly)
{
  digitalWrite(pn, LOW);
  delay(dly);  
}

/* Ligar um elemento em shift register */

void onSr(byte pn, int dly)
{
  sr.set(pn, HIGH);
  delay(dly);  
}

/* Desligar um elemento em shift register */

void offSr(byte pn, int dly)
{
  sr.set(pn, LOW);
  delay(dly);  
}

/* Desligar semaforo de treinamento */

void offSema()
{
  sr.set(0, LOW);
  sr.set(1, LOW);
  sr.set(2, LOW);
}

/* Ligar camada de sinapses W */

void onW(int dly)
{
  onSr(9,0);
  onSr(10,0);
  onSr(11,0);
  tick();
  delay(dly);
}

/* Desligar camada de sinapses W */

void offW(int dly)
{
  offSr(9,0);
  offSr(10,0);
  offSr(11,0);
  tick();
  delay(dly);
}

/* Piscar a camada de sinapses W */

void blkW(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onW(dly);

    offW(dly);
  }
}

/* Ligar a camada de sinpses Q */

void onQ(int dly)
{
  onSr(12,0);
  onSr(13,0);
  onSr(14,0);
  tick();
  delay(dly);
}

/* Desligar a camada de sinapses Q */

void offQ(int dly)
{
  offSr(12,0);
  offSr(13,0);
  offSr(14,0);
  tick();
  delay(dly);  
}

/* Piscar camada de sinapses Q */

void blkQ(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onQ(dly);

    offQ(dly);
  }
}

/* Ligar camada de entrada */

void onX(int dly)
{
  onSr(3,0);
  onSr(4,0);
  onSr(5,0);
  tick();
  delay(dly);
}

/* Desligar camada de entrada */

void offX(int dly)
{
  offSr(3,0);
  offSr(4,0);
  offSr(5,0);
  tick();
  delay(dly);
}

/* Piscar camada de entrada */

void blkX(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onX(dly);

    offX(dly);
  }
}

/* */

void onY(int dly)
{
  onSr(6,0);
  onSr(7,0);
  onSr(8,0);
  tick();
  delay(dly);
}

void offY(int dly)
{
  offSr(6,0);
  offSr(7,0);
  offSr(8,0);
  tick();
  delay(dly);
}

void blkY(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onY(dly);

    offY(dly);
  }
}

void onH(int dly)
{
  digitalWrite(h1Pin,HIGH);
  digitalWrite(h2Pin,HIGH);
  digitalWrite(h3Pin,HIGH);
  tick();
  delay(dly);
}

void offH(int dly)
{
  digitalWrite(h1Pin,LOW);
  digitalWrite(h2Pin,LOW);
  digitalWrite(h3Pin,LOW);
  tick();
  delay(dly);
}

void blkH(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onH(dly);

    offH(dly);
  }
}


void onO(int dly)
{
  digitalWrite(o1Pin,HIGH);
  digitalWrite(o2Pin,HIGH);
  digitalWrite(o3Pin,HIGH);
  tick();
  delay(dly);
}

void offO(int dly)
{
  digitalWrite(o1Pin,LOW);
  digitalWrite(o2Pin,LOW);
  digitalWrite(o3Pin,LOW);
  tick();
  delay(dly);
}

void blkO(int dly)
{
  for(byte i=0;i<rep;i++)
  {
    onO(dly);

    offO(dly);
  }
}

void trn(int dly)
{
  onX(dly);
  offX(0);

  onW(dly);
  offW(0);

  onH(dly);
  offH(0);
  
  onQ(dly);
  offQ(0);

  onO(dly);
  offO(0);

  onY(dly);
  offY(0);

  onO(dly);
  offO(0);

  onQ(dly);
  offQ(0);
  
  onH(dly);
  offH(0);

  onW(dly);
  offW(0);

  onX(dly);
  offX(0);
}

void explain(String txt, byte row, boolean cls, byte tipo)
{
  if(cls)
  {
    lcd.clear();

    switch(tipo)
    {
      case ff : cPrint(0,F("[ Feedforward ]")); break;
      case bp : cPrint(0,F("[ Backpropagation ]")); break;
      case el : cPrint(0,F("[ Elementos ]")); break;
    }
  }

  cPrint(row,txt);
}

void feedForwardAnimate(int dly)
{
  cute.play(S_MODE1);
  
  // 1

  explain(F("Valor Entrada 1"),1,true,ff);
  
  blkSr(3,dly);

  onSr(3,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(9,dly);

  onSr(9,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkH(dly);

  onH(dly);

  offSr(3,0);

  offSr(9,0);

  offH(0);

  // 2

  explain(F("Valor Entrada 2"),1,true,ff);

  blkSr(4,dly);

  onSr(4,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(10,dly);

  onSr(10,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkH(dly);

  onH(dly);

  offSr(4,0);

  offSr(10,0);

  offH(0);

  // 3

  explain(F("Valor Entrada 3"),1,true,ff);

  blkSr(5,dly);

  onSr(5,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(11,dly);

  onSr(11,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkH(dly);

  onH(dly);

  offSr(5,0);

  offSr(11,0);

  offH(0);

  explain(F("Ativacao Sigmoide"),1,true,ff);

  blkH(dly);

  blkH(dly);

  // 4

  explain(F("Axonio Neuronio 1"),1,true,ff);

  blkPn(h1Pin,dly);

  onPn(h1Pin,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(12,dly);

  onSr(12,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkO(dly);

  onO(dly);

  offPn(h1Pin,0);

  offSr(12,0);

  offO(0);

  // 5

  explain(F("Axonio Neuronio 2"),1,true,ff);

  blkPn(h2Pin,dly);

  onPn(h2Pin,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(13,dly);

  onSr(13,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkO(dly);

  onO(dly);

  offPn(h2Pin,0);

  offSr(13,0);

  offO(0);

  // 6

  explain(F("Axonio Neuronio 3"),1,true,ff);

  blkPn(h3Pin,dly);

  onPn(h3Pin,dly);

  explain(F("vezes Sinapses"),2,false,ff);

  blkSr(14,dly);

  onSr(14,dly);

  explain(F("soma nos Dendritos"),3,false,ff);

  blkO(dly);

  onO(dly);

  offPn(h3Pin,0);

  offSr(14,0);

  offO(0);

  explain(F("Ativacao Sigmoide"),1,true,ff);

  blkO(dly);

  blkO(dly);

  // 7

  explain(F("Axonio Saida 1"),1,true,ff);

  blkPn(o1Pin,dly);

  explain(F("Axonio Saida 2"),1,true,ff);

  blkPn(o2Pin,dly);

  explain(F("Axonio Saida 3"),1,true,ff);

  blkPn(o3Pin,dly);

  cute.play(S_CONNECTION);

  delay(500);
}

void backPropagationAnimate(int dly)
{
  cute.play(S_MODE3);

  explain(F("u1 - u3"),1,true,bp);
  explain(F("Calculo dos Erros"),2,false,bp);
  explain(F("nas Saidas"),3,false,bp);

  for(byte i=0;i<3;i++)
  {
    for(byte j=0;j<rep;j++)
    {
      tick();

      onPn((9+i),dly);
      onSr(6+i,dly);

      tick();

      offPn(9+i,dly);
      offSr(6+i,dly);

    }
  }

  onY(0);
  onO(0);

  explain(F("f1 - f3"),1,true,bp);
  explain(F("Calculo dos Erros"),2,false,bp);
  explain(F("na Camada Escondida"),3,false,bp);

  for(byte i=0;i<3;i++)
  {
    for(byte j=0;j<rep;j++)
    {
      tick();

      byte k = i;

      if(i>0) k++;

      onPn(3+k,dly);
      onSr(12+i,dly);

      tick();

      offPn(3+k,dly);
      offSr(12+i,dly);

    }
  }

  onH(0);

  explain(F("deltaQ1 - deltaQ9"),1,true,bp);
  explain(F("Calculo do Delta"),2,false,bp);
  explain(F("nas Sinapses Q1-9"),3,false,bp);

  blkQ(dly);

  
  explain(F("deltaW1 - deltaW9"),1,true,bp);
  explain(F("Calculo do Delta"),2,false,bp);
  explain(F("nas Sinapses W1-9"),3,false,bp);

  blkW(dly);

  allOff();
  
  explain(F("Execucao de Ajustes"),1,true,bp);
  explain(F("Calculo do Delta"),2,false,bp);
  explain(F("nas Sinapses Q"),3,false,bp);

  blkQ(dly);
  blkW(dly);

  cute.play(S_CONNECTION);

  delay(500);
}

void elementosAnimate(int dly)
{
  cute.play(S_MODE2);
  
  explain(F("x1 - x3"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Entradas"),3,false,el);

  blkX(dly);

  explain(F("w1 - w9"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Sinapses"),3,false,el);

  blkW(dly);

  explain(F("h1 - h3"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Neuronios Escondidos"),3,false,el);

  blkH(dly);

  explain(F("q1 - q9"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Sinapses"),3,false,el);

  blkQ(dly);

  explain(F("o1 - o3"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Saidas Calculadas"),3,false,el);

  blkO(dly);

  explain(F("y1 - y3"),1,true,el);
  explain(F("Camada de"),2,false,el);
  explain(F("Saidas Desejadas"),3,false,el);

  blkY(dly);

  cute.play(S_CONNECTION);

  delay(500);
}

void clearJBuffer()
{
  while(joyMoved())
    readInterface();
}

void testarHardware()
{
  readInterface();

  allOn();

  cute.play(S_CONNECTION);

  while(button3Val==1)
  {
    readInterface();
    delay(300);
    dumpInterfaceValues();
  }

  cute.play(S_HAPPY);

  while(button3Val==0)
    readInterface();

  allOff();

  showMenu();
  
}

void showMenuDemonstrar()
{
  lcd.clear();
  
  cPrint(0,F("[ Menu Demonstrar ]"));
  cPrint(1,F("Elementos"));
  cPrint(2,F("Feedforward"));
  cPrint(3,F("Backpropagation"));
}

void menuDemonstrar()
{
  byte menuPos = 0;

  clearJBuffer();

  showMenuDemonstrar();

  cPrint(1,F("> Elementos <"));
  
  while(true)
  {
    readInterface();
    
    if(joyMoved())
    {
      tick();
      
      showMenuDemonstrar();

      if(joyXVal>900)
        menuPos++;
      if(joyXVal<100)
        menuPos--;
      if(joyYVal>900 || joyButtonVal==0)
      {
          lcd.clear();
          
          switch(menuPos)
          {
            case 0: elementosAnimate(700); break;
            case 1: feedForwardAnimate(300); break;
            case 2: backPropagationAnimate(300); break;
          }
      }
      if(menuPos<0) menuPos=0;
      if(menuPos>2) menuPos=2;

      showMenuDemonstrar();

      switch(menuPos)
      {
        case 0 : cPrint(1,F("> Elementos <")); break;
        case 1 : cPrint(2,F("> Feedforward <")); break;
        case 2 : cPrint(3,F("> Backpropagation <")); break; 
      }

      if(joyYVal<100 || button3Val==0)
      {
        lcd.clear();

        showMenu();

        return;
      }

      clearJBuffer();
      
    }

  }
}

void initPares()
{
  X[0][0]=1;
  X[0][1]=0;
  X[0][2]=0;

  Y[0][0]=1;
  Y[0][1]=0;
  Y[0][2]=0;

  X[1][0]=0;
  X[1][1]=1;
  X[1][2]=0;

  Y[1][0]=1;
  Y[1][1]=0;
  Y[1][2]=1;

  X[2][0]=0;
  X[2][1]=0;
  X[2][2]=1;

  Y[2][0]=1;
  Y[2][1]=0;
  Y[2][2]=0;

  X[3][0]=1;
  X[3][1]=1;
  X[3][2]=0;

  Y[3][0]=0;
  Y[3][1]=1;
  Y[3][2]=1;

  X[4][0]=0;
  X[4][1]=1;
  X[4][2]=1;

  Y[4][0]=1;
  Y[4][1]=0;
  Y[4][2]=1;

  X[5][0]=1;
  X[5][1]=0;
  X[5][2]=1;

  Y[5][0]=1;
  Y[5][1]=1;
  Y[5][2]=1;

}

void initRec()
{
  X[6][0]=0;
  X[6][1]=0;
  X[6][2]=0;

  Y[6][0]=0;
  Y[6][1]=0;
  Y[6][2]=0;
}

void zeraEntradasSaidasDesejadas()
{
  for(int i=0;i<A;i++)
    x[i]=0;
  
  for(int k=0;k<C;k++)
    y[k]=0; 
}

float randomF()
{
  float flag = 0.0;

  flag = random(-10000,10000);

  flag = float(flag)/100000;

  return flag;
}

void initRede()
{
  zeraEntradasSaidasDesejadas();
  
  /** inicializar sinapses com valores entre -0.1 e 0.1 **/
  
  for(int i=0;i<A;i++)
    for(int j=0;j<B;j++)
      w[i][j]=randomF();
      
  for(int j=0;j<B;j++)
    for(int k=0;k<C;k++)
      q[j][k]=randomF();
}

void dumpRede()
{
  Serial.println(F("Camada de Entradas (x[i])"));
  
  for(int i=0;i<A;i++)
    Serial.print("[" + (String)i + "] = " + (String)x[i] + "   ");
  
  Serial.println("\n");
  
  
  
  Serial.println(F("Camada Escondida (h[j])"));
  
  for(int j=0;j<B;j++)
    Serial.print("[" + (String)j + "] = " + (String)h[j] + "   ");
  
  Serial.println("\n");
  
  
  
  Serial.println(F("Camada de Saidas (o[k])"));
  
  for(int k=0;k<C;k++)
    Serial.print("[" + (String)k + "] = " + (String)o[k] + "   ");
  
  Serial.println("\n");
  
  
  
  Serial.println(F("Camada de Saidas Desejadas (y[k])"));
  
  for(int k=0;k<C;k++)
    Serial.print("[" + (String)k + "] = " + (String)y[k] + "   ");
  
  Serial.println("\n");
  
  
  
  Serial.println(F("Sinapses entre Entradas e Camada Escondida (w[i][j])"));
  
  for(int i=0;i<A;i++)
  {
    for(int j=0;j<B;j++)
      Serial.print("[" + (String)i + "/" + (String)j + "] = " + (String)w[i][j] + "   ");
   
    Serial.println();
  }
  
  Serial.println();
  
  
  
  Serial.println(F("Sinapses entre Camada Escondida e Saidas (q[j][k])"));
      
  for(int j=0;j<B;j++)
  {
    for(int k=0;k<C;k++)
      Serial.print("[" + (String)j + "/" + (String)k + "] = " + (String)q[j][k] + "   ");
    
    Serial.println();
  }
  
  Serial.println();
  
  Serial.println(F("------------------------------------------------------\n"));
}

float sigmoide(float x)
{
  return (1 / (1 + exp(-x)));
}

void zeraEscondidosSaidas()
{
  for(int j=0;j<B;j++)
    h[j]=0;
  
  for(int k=0;k<C;k++)
    o[k]=0;
}

/**
 * Funcionar a rede
 */

void funcionarRede()
{
  zeraEscondidosSaidas();
  
  for(byte i=0;i<A;i++)
    for(byte j=0;j<B;j++)
      h[j]=h[j]+x[i]*w[i][j];
  
  for(byte j=0;j<B;j++)
    h[j]=sigmoide(h[j]);
  
  for(byte j=0;j<B;j++)
    for(byte k=0;k<C;k++)
      o[k]=o[k]+h[j]*q[j][k];
  
  for(byte k=0;k<C;k++)
    o[k]=sigmoide(o[k]);
  
}

/**
 * Executar o algoritmo de Backpropagation
 */

void backPropagation()
{
  /** Calculo dos erros nas saidas **/
  
  for(byte k=0;k<C;k++)
    u[k]=o[k]*(1.0-o[k])*(y[k]-o[k]);
  
  /** Calculo dos erros na camada escondida **/
  
  for(byte j=0;j<B;j++)
  {
    s[j]=0.0;
    
    for(byte k=0;k<C;k++)
      s[j]=s[j]+u[k]*q[j][k];
    
    f[j]=h[j]*(1.0-h[j])*s[j];
  }
  
  /** Delta nas sinapses entre camada escondida e saida **/
  
  for(byte j=0;j<B;j++)
    for(byte k=0;k<C;k++)
      deltaQ[j][k]=N*u[k]*h[j];
  
  /** Delta nas sinapses entre entradas e camanda escondida **/
  
  for(byte i=0;i<A;i++)
    for(byte j=0;j<B;j++)
      deltaW[i][j]=N*f[j]*x[i];
  
  /** Execucao de ajustes **/
  
  for(byte j=0;j<B;j++)
    for(byte k=0;k<C;k++)
      q[j][k]=q[j][k]+deltaQ[j][k];
  
  for(byte i=0;i<A;i++)
    for(byte j=0;j<B;j++)
      w[i][j]=w[i][j]+deltaW[i][j];
  
}

boolean exitButtonPressed()
{
  byte b = digitalRead(button3Pin);

  if(b==0)
    return true;

  return false;
}

void updateLeds(byte p, byte c)
{
  allOff();

  for(byte i=0;i<3;i++)
  {
    if(X[p][i]==1)
      onSr(3+i,0); else offSr(3+i,0);

    if(Y[p][i]==1)
       onSr(6+i,0); else offSr(6+i,0);

  }

  onPn(9+c,40);
  offPn(9+c,40);
  onPn(9+c,0);
}

void updateDesc(byte p, byte c)
{
  lcd.setCursor(1,2);

  p++;
  c++;

  lcd.print("Par: "); 
  lcd.print(p);
  lcd.print("  Valores: "); 
  lcd.print(c);
  
}

void updateDesc2(byte c)
{
  lcd.setCursor(1,2);

  c++;

  lcd.print("  Valores: "); 
  lcd.print(c);
  
}

void programarRede()
{
  byte pos = 0, chng = 0, jd = 0, b1 = 1, b2 = 1;
  
  cPrint(0,F("[ Programar Rede ]"));

  cPrint(3,F("[In] Program [Out]"));

  cute.play(S_CONNECTION);

  while(exitButtonPressed());

  jd=NONE;

  updateLeds(pos,chng);
  updateDesc(pos,chng);

  while(exitButtonPressed()==false)
  {

   jd = getJoyDir();

   if(jd!=NONE)

   {

    switch(jd)
    {
      case UP : if(chng>0) chng--; tick(); break;
      case DOWN : if(chng<2) chng++; tick(); break;
      case LEFT : if(pos>0) pos--; tick(); break;
      case RIGHT : if(pos<5) pos++; tick(); break;
    }

    updateLeds(pos,chng);

      updateDesc(pos,chng);

      while(getJoyDir()!=NONE);

   }

   b1 = digitalRead(button1Pin);
   b2 = digitalRead(button2Pin);

   if(b1==0)
   {
      if(X[pos][chng]==0)
          X[pos][chng]=1; else X[pos][chng]=0;

          tick();

    updateLeds(pos,chng);

      while(b1==0)
        b1 = digitalRead(button1Pin);
   }

   if(b2==0)
   {
      if(Y[pos][chng]==0)
          Y[pos][chng]=1; else Y[pos][chng]=0;

          tick();

    updateLeds(pos,chng);

      while(b2==0)
        b2 = digitalRead(button2Pin);
   }

    
    updateLeds(pos,chng);

    
  }

  cute.play(S_HAPPY);

  allOff();

  delay(500);

  showExecutar();
}

void treinarRede()
{
  float e = 0.0;

  int cnt = 0;

  silent = true;

  cPrint(0,F("[ Treinar Rede ]"));

  cute.play(S_CONNECTION);

  while(exitButtonPressed());

  onSr(0,0);

  cPrint(1,F("Escolher"));

  lcd.setCursor(3,2);

  lcd.print(F("Taxa (N): "));

  cPrint(3,F("[<<] Program [>>]"));

  while(exitButtonPressed()==false)
  {
    lcd.setCursor(13,2);

    lcd.print(N);

    readInterface();

    if(button1Val==0 && N>0.05)
    {
      N=N-0.05;
    }

    if(button2Val==0 && N<0.95)
    {
      N=N+0.05;
    }

    delay(250);
  }

  cute.play(S_CONNECTION);

  while(exitButtonPressed())
  {
    readInterface();
  }

  lcd.clear();

  cPrint(0,F("[ Treinar Rede ]"));

  cPrint(3,F("[Exit] Terminar"));

  while(exitButtonPressed()==false && cnt<32000)
  {
    cnt++;
    
    for(int k=0;k<6;k++)
    {
      
      copyIO(k);

      funcionarRede();

      e = e + erroMedio();
      
      backPropagation();
   
    }

    e = e / 6;

    if(cnt%20==0)
    {
      lcd.setCursor(1,1);
  
      lcd.print(F("Erro: "));
  
      lcd.print(e);
  
      lcd.print(F(" N: "));
  
      lcd.print(N);
  
      lcd.setCursor(1,2);
  
      lcd.print(F("Passo: "));
  
      lcd.print(cnt);
    }

    if(cnt%80==0)
    {
      trn(30);

      offSema();

      if(e<0.06)
        onSr(2,0); else
          if(e<0.2)
            onSr(1,0); else
              onSr(0,0);
    }   

    e =0.0;
  }

  if(cnt>=32000)
    while(exitButtonPressed()==false)
      readInterface();

  allOn();

  cute.play(S_HAPPY);

  delay(1000);

  allOff();

  showExecutar();

  silent=false;

  delay(500);
}

void copyIO(byte p)
{
  for(byte i=0;i<3;i++)
  {
    x[i]=X[p][i];
    y[i]=Y[p][i];
  }
}

void showH()
{
  byte valor = 0;
  byte diff = 0;
  
  for(byte i=0;i<3;i++)
  {
    valor=(byte)(h[i]*50.0);

    Serial.print(valor);

    if(i==0) diff=0; else diff=1;

    analogWrite(3+i+diff,valor);
  }
}

void showO()
{
  byte valor = 0;
  
  for(byte i=0;i<3;i++)
  {
    valor=(byte)(o[i]*50.0);

    Serial.print(valor);

    analogWrite(9+i,valor);
  }
}

void animateFuncionar(int dly)
{
  blkW(dly);

  blkH(dly);

  showH();

  blkQ(dly);

  blkO(dly);

  showO();
}

float erroMedio()
{
  float flag=0;

  for(byte i=0;i<3;i++)
  {
    flag=flag+abs(y[i]-o[i]);  
  }

  flag=flag/3;

  return flag;
}

void reconhecerRede()
{
  initRec();
  
  cPrint(0,F("[ Reconhecer ]"));

  cute.play(S_CONNECTION);

  byte pos = 0, chng = 0, jd = 0, b1 = 1, b2 = 1;
  
  while(exitButtonPressed());

  jd=NONE;

  updateLeds(pos,chng);
  updateDesc2(chng);

  while(exitButtonPressed()==false)
  {

   jd = getJoyDir();

   if(jd!=NONE)

   {

    switch(jd)
    {
      case UP : if(chng>0) chng--; tick(); break;
      case DOWN : if(chng<2) chng++; tick(); break;
    }

    updateLeds(6,chng);

      updateDesc2(chng);

      while(getJoyDir()!=NONE);

   }

   b1 = digitalRead(button1Pin);
   b2 = digitalRead(button2Pin);

   if(b1==0)
   {
      if(X[6][chng]==0)
          X[6][chng]=1; else X[6][chng]=0;

          tick();

    updateLeds(6,chng);

      while(b1==0)
        b1 = digitalRead(button1Pin);
   }

   if(b2==0)
   {
      if(Y[6][chng]==0)
          Y[6][chng]=1; else Y[6][chng]=0;

          tick();

    updateLeds(6,chng);

      while(b2==0)
        b2 = digitalRead(button2Pin);
   }

    
    updateLeds(6,chng);

    
  }

  offO(0);

  copyIO(6);

  lcd.clear();

  cPrint(0,F("[ Reconhecer ]"));

  funcionarRede();

  animateFuncionar(200);

  lcd.setCursor(2,2);

  lcd.print(F("Erro: "));

  lcd.print(erroMedio());

  while(exitButtonPressed());

  while(exitButtonPressed()==false);

  cute.play(S_HAPPY);

  allOff();

  delay(500);

  showExecutar();

  delay(500);
}

void showExecutar()
{
  lcd.clear();
  
  cPrint(0,F("[ Menu Executar ]"));
  cPrint(1,F("Programar"));
  cPrint(2,F("Treinar"));
  cPrint(3,F("Reconhecer"));
}

void menuExecutar()
{
    byte menuPos = 0;
  
  lcd.clear();

  showExecutar();

  cPrint(1,F("> Programar <"));

  clearJBuffer();
  
  while(true)
  {
    readInterface();
    
    if(joyMoved())
    {
      tick();
      
      showExecutar();

      if(joyXVal>900)
        menuPos++;
      if(joyXVal<100)
        menuPos--;
      if(joyYVal>900 || joyButtonVal==0)
      {
          lcd.clear();
          
          switch(menuPos)
          {
            case 0: programarRede(); break;
            case 1: treinarRede(); break;
            case 2: reconhecerRede(); break;
          }
      }
      if(menuPos<0) menuPos=0;
      if(menuPos>2) menuPos=2;

      switch(menuPos)
      {
        case 0 : cPrint(1,F("> Programar <")); break;
        case 1 : cPrint(2,F("> Treinar <")); break;
        case 2 : cPrint(3,F("> Reconhecer <")); break; 
      }

      if(joyYVal<100 || button3Val==0)
      {
        lcd.clear();

        showMenu();

        return;
      }

      clearJBuffer();
      
    }

  }
}

void showMenu()
{
  lcd.clear();
  
  cPrint(0,F("[ Menu Principal ]"));
  cPrint(1,F("Demonstrar"));
  cPrint(2,F("Executar"));
  cPrint(3,F("Testar Hardware"));
}

void menu()
{
  byte menuPos = 0;
  
  lcd.clear();

  showMenu();

  cPrint(1,F("> Demonstrar <"));
  
  while(true)
  {
    readInterface();
    
    if(joyMoved())
    {
      tick();
      
      showMenu();

      if(joyXVal>900)
        menuPos++;
      if(joyXVal<100)
        menuPos--;
      if(joyYVal>900 || joyButtonVal==0)
      {
          lcd.clear();
          
          switch(menuPos)
          {
            case 0: menuDemonstrar(); break;
            case 1: menuExecutar(); break;
            case 2: testarHardware(); break;
          }
      }
      if(menuPos<0) menuPos=0;
      if(menuPos>2) menuPos=2;

      switch(menuPos)
      {
        case 0 : cPrint(1,F("> Demonstrar <")); break;
        case 1 : cPrint(2,F("> Executar <")); break;
        case 2 : cPrint(3,F("> Testar Hardware <")); break; 
      }

      clearJBuffer();
      
    }

  }
}

void loop() {

  menu();
 
}
