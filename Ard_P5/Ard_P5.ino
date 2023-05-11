//**********************************************************************
//*********ESPECTROFOTOMETRO DE TRANSMISSÃO: FENTO (Visivel)************
//**********************************************************************

/*
    Documentação adicional:
    + https://docs.arduino.cc/built-in-examples/communication/MultiSerialMega
    + https://victorvision.com.br/docs/arduino-library/
      + Expanda o botão "Classe LCM" na tabela direita da página.
    + UnicViewAD General Reference. É necessário para entender de modo geral como usar o text input, etc.

    ******* MANUAL DE USO *******
    1. Conecte um cartão SD ao Mega do equipamento
    2. Ligue o equipamento
    3. Faça o upload deste código-fonte
    4. Aperte no botão de início. O cursor do equipamento será calibrado
    5. Se quiser, ajuste os limites do lambda usando o botão no menu principal
    6. Aperte I0 para definir a linha de base. Todos os outros botões não têm uso sem antes fazer isso.
        I0 será plotado em unidades arbitrárias de 0 a 1023
    7. EM seguida, aperte T[%]. Isto plotará a transimissão.
    8. Até 3 T podem ser plotados simultaneamente: azul, vermelho e verde.
        T será plotado em percentual de 0 a 100%
    9. Infelizmente, a rotina de salvar não está funcionando por causa de problemas de leitura do cartão SD.
        Ainda assim, os botões são funcionais e o arquivo deve ser gerado, mas sem dados salvos.
    10. Os demais botões do menu principal são intuitivos.
    ******************************

    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    !!!!!! ATENÇÃO !!!!!!!!!!!!!!!!!!!!!!
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    O cartão SD tem problemas de leitura no Arduino Mega ligado ao equipamento.
    Esses problemas não existem no outro Arduino Mega da bancada ou em qualquer UNO.
    Infelizmente, o cartão SD parece se corromper todas as vezes que é usado com o Mega ligado ao equipamento.
    Não entendo qual é o motivo disso.
    Estou usando a biblioteca SdFat 1.1.4 (link: https://github.com/greiman/SdFat/releases/tag/1.1.4) com 3.3 Volts porque
    a biblioteca SD comum, com 5 Volts, não funcionava em qualquer Mega testado. O Luiz Antônio estava comigo na sala, pode confirmar.

    O programa apenas será inicializado se um cartão SD estiver conectado, no entanto. Por mais que o cartão SD não seja usado,
    ele precisa estar conectado para o programa rodar.

    Pinagem:
      SD:
        Mosi: 51
        Miso: 50
        SCK: 52
        CD: 53
        Use 3.3 Volts
*/
#include <UnicViewAD.h> //Biblioteca comunic. display
// Arduino Mega tem 4 portas Serial. A porta 0 é do cabo Serial. As outras 3 portas podem ser usadas por componentes externos.
// O display será inicializado na porta Serial 3: TX3, RX3.
LCM Lcm(Serial3);

// Library SdFat 1.1.4
#include <SPI.h>
#include "SdFat.h"
SdFat SD;

#define SD_CS_PIN 4
File arquivo;


/*
 * Configuração das variáveis do Display:
 */
// LockScreen
LcmVar startButton(10);

// Botão do plot:
LcmVar plotButton(100);

// Plot Vars:
LcmVar ytick1(310); // Estes são os números nos "ticks" do gráfico
LcmVar ytick2(311);
LcmVar xtick1(312);
LcmVar xtick2(313);
LcmVar xtick3(314);
LcmVar xtick4(315);

LcmVar xBox(320); // Estas são as coordenadas x,y no canto superior direito
LcmVar yBox(321);

LcmString yLabel(330, 20); // Menciona se está medindo I0 ou T[%]
LcmString plotInfo(360, 20); // Diz o que está sendo medido

// Salvar SD:
LcmString fileNameInput(402, 16);
LcmString fileNameDisplay(422, 16);
LcmVar SDbutton(400);

// Ajustar lambda
  // Numeric Inputs:
LcmVar lcmLambdaMinInput(502);
LcmVar lcmLambdaMaxInput(501);
  // Numeric Displays:
LcmVar lcmLambdaMinDisplay(505);
LcmVar lcmLambdaMaxDisplay(504);

LcmVar lambdaReturn(503); // Botão para sair desta tela

// Mensagens gerais
LcmString lcmMsg(1100, 100);


//****** Constantes nomeadas
#define LED        13             //***LIGAÇOES ORIGINAIS
#define Sentido_1  26             //***FIO AZUL (8)
#define Pulso_1    28             //***FIO CINZA(9)
#define Sensor_2   22             //***FIO VERMELHO(2)
#define Chave_1    24             //***FIO lARANJA (4)
// Chave_1 é o pino referente
// ao 1º sensor também.
#define  CA        15             //CONVERSOR ANALOGICO (0)


//***** Variáveis gerais
const int N_medias = 100; // Cada medida é efetuada 100 vezes e uma média é efetuada
const int plotPoints = 341; // Quantidade de pontos do plot é fixa em 341. Depende da extensão horizontal de cada objeto TrendCurveDisplay.
int posicao = 540;              // Define a posição atual do contador, atualizada dinamicamente. Inicia em 540 porque é essa a posição quando o cursor detecta o sensor óptico.

int lambda_min = 500, lambda_max = 550;
int tot_medicoes; // Total de medidas. Número dinâmico, depende de lambda_min e lambda_max.
int I0[650]; // Array das medições I0. O número esdrúxulo do array é para armazenar diferentes valores de tot_medicoes
int I[650]; // Array das medições I.
bool I0_plotted = false; // Variável de controle; define se I0 já foi medido
bool I_plotted = false; // Variável de controle; define se I já foi medido
int currentChannel = 0; // Define o canal atual do plot

// A taxa de passos por unidade de comprimento de onda deslocado.
// São necessários ~4.4 passos para cada nm deslocado na detecção.
// Esta quantidade é puramente experimental.
const float rate = 1000. / 229.; // dN/dl = 1000passos/229nm

// Uso geral:
unsigned long t_i;

void setup()
{
  Serial.begin(9600);
  Serial3.begin(115200);
  Lcm.begin();
  t_i = millis();
  //*** Garantir o reinício completo do display
  // Por um total de 1s, força o reinício do display.
  // Isto contorna o problema do Lcm.write recebendo dados indevidos.
  while (millis() - t_i < 1000) {
    Lcm.resetLCM();
    Lcm.changePicId(0);
  }
  //***
  pinMode(Sentido_1, OUTPUT);      //CONTROLE DO MOTOR DE PASSO
  pinMode(Pulso_1, OUTPUT);        //CONTROLE DO MOTOR DE PASSO
  pinMode(Sensor_2, INPUT);        //PONTO DE REFERENCIA DO MONOCROMADOR(SENSOR OTICO)
  pinMode(Chave_1, INPUT_PULLUP);  //FIM DE CURSO
  analogReference(DEFAULT);
  // Rotina inicial: mapear o cursor, iniciar quando a tela for tocada
  lockStart();
  // Imprime mensagem ao usuário:
  displayMessage("Inicie o programa medindo I0", 2000,true);
}

void loop()
{
  plotInfo.write("Pressione algo", 20);

  // Aguarda até que um botão seja pressionado
  if (plotButton.available()) {

    byte opcao = plotButton.getData();
    Serial.println(opcao);
    switch (opcao)
    {
      // Botão 1: Lê e plota I0
      // Limpa o TrendCurve. Insere os limites corretos. Mede e plota simultaneamente.
      case 1:
        Lcm.changePicId(12); // Troca para a tela onde os botões estão bloqueados
        cleanPlot(0, 1023, 0); // Limpa todos os 3 canais
        cleanPlot(0, 1023, 1);
        cleanPlot(0, 1023, 2);
        currentChannel = 0; // Redefine o canal atual para 0
        plotInfo.write("Medindo I0", 20);
        yLabel.write("  I0    [UA]", 20); // Atualiza o label vertical do plot
        Medir(lambda_min, lambda_max, I0, 'b'); // Mede I0 indicando que é a linha de base ('b')
        // Atualiza as variáveis de controle
        I0_plotted = true; 
        I_plotted = false;
        // Retorna ao menu principal
        Lcm.changePicId(1);
        break;

      case 2:
        // Botão 2: Lê I e plota T
        // Apenas será pessionável se existir I0.
        if (I0_plotted) {
          Lcm.changePicId(12); // Troca para a tela onde os botões estão bloqueados
          cleanPlot(0, 100, currentChannel); // Limpa o canal atual para que a curva seja plotada com o canal atual vazio
          plotInfo.write("Medindo T", 20);
          yLabel.write("  T    [%]", 20);
          Medir(lambda_min, lambda_max, I, 't'); // Mede I indicando que é a amostra de transmissão ('t')
          Lcm.changePicId(3); // Troca para a tela de salvar medição
          currentChannel = currentChannel + 1; // Atualiza o canal e garante que sempre está de 0 a 2
          if (currentChannel > 2) currentChannel = 0;
          I_plotted = true; //Atualiza que I foi medido e T foi plotado
        }
        else {
          displayMessage("Aperte I0 primeiro.", 2000,true);
        }
        break;

      case 3:
        // Alterar Lambda
        adjustLambda(true); // Ajusta lambda e força deslocamento do cursor
        I0_plotted = false; // "Reseta" I0 e I
        I_plotted = false;
        displayMessage("Recalcule I0 e T", 3000,false);
        Lcm.changePicId(1); // Retorna para a tela principal
        break;

      case 4:
        // Limpa o plot com os comprimentos de onda atuais
        cleanPlot(0, 0, 0);delay(50);
        cleanPlot(0, 0, 1);delay(50);
        cleanPlot(0, 0, 2);delay(50);
        break;

      case 5:
        // Salva T no cartão SD
        // Apenas estará disponível se tanto I0 como I existirem
        if (I0_plotted and I_plotted) {
          saveToSDScreen();
        }
        else {
          displayMessage("Aperte I0 primeiro e depois T.", 2000,true);
        }
        break;
        
      case 6:
        // Imprime I0 no serial
        // Apenas estará disponível se I0 existir
        if (I0_plotted) {
          Serial.println("\n\n\n");
          Serial.println("Dados do vetor I0:");
          Serial.print("Num medidas: "); Serial.println(tot_medicoes);
          printVect(I0);
          Serial.println("\n\n\n");
          displayMessage("Dados de I0 impressos na Serial",2000,true);
        }
        else {
          displayMessage("Aperte I0 primeiro.", 2000,true);
        }
        break;
        
      case 7:
        // Imprime I no serial
        // Apenas estará disponível se I0 e I existirem
        if (I0_plotted and I_plotted) {
          Serial.println("\n\n\n");
          Serial.println("Dados do vetor I:");
          Serial.print("Num medidas: "); Serial.println(tot_medicoes);
          printVect(I);
          Serial.println("\n\n\n");
          displayMessage("Dados de I impressos na Serial",2000,true);
        }
        else {
          displayMessage("Aperte I0 e T primeiro.", 2000,true);
        }
        break;
        
      case 8:
        // Imprime T no serial
        // Apenas estará disponível se I0 e I existirem
        if (I0_plotted and I_plotted) {
          Serial.println("\n\n\n");
          Serial.println("Dados da transmissao T=I/I0:");
          Serial.print("Num medidas: "); Serial.println(tot_medicoes);
          printTransm();
          Serial.println("\n\n\n");
          displayMessage("Dados de T impressos na Serial",2000,true);
        }
        else {
          displayMessage("Aperte I0 e T primeiro.", 2000,true);
        }
        break;
    }

  }
}

void Medir(int lambda_min, int lambda_max, int vector[650], char selection)
{
  /*
     Esta rotina fará a leitura de um comprimento de onda (nm) a cada
     `skip_steps` passos.
     Portanto, o total de passos corridos é dado por
        pontos*skip_steps = passos = (delta lambda)*rate;
     Isto implica pontos = (delta lambda)*rate/skip_steps

     Ou seja, o total de pontos medidos depende dos limites de nm.

     Automaticamente faz o plot dos pontos.

     A plotagem "ao vivo" dos dados é consideravelmente mais complicada.
     Efetivamente, dada a diferença de pontos entre os medidos e os do plot, preciso saber quantas vezes
     uma determinada medida deve ser plotada. Pode ser plotado 0 vezes, 10 vezes, 2 vezes, etc.
     Para cada ponto do plot, há um inteiro correspondente ao índice da medida. Supondo relação linear,
     delta(imed)/delta(iplot) = (Nmed-1)/(Nplot-1) = conversion.
     Logo, imed = round(conversion*iplot).
     Portanto, diferentes iplot podem resultar num mesmo imed, se Npts > Nmed. Essa quantidade é, portanto, o total de vezes
     que preciso plotar a medição atual.
     Se Npts < Nmed, significa que certas medições serão puladas, ou seja, alguns índices medidos têm contagem nula.
  */
  int skip_steps = 5; // Lê 1 a cada 5 passos.

  float medida;

  tot_medicoes = (lambda_max - lambda_min) * rate / skip_steps; // Total de pontos nm medidos
  const float conversion = float(tot_medicoes - 1) / float(plotPoints - 1); // Ver documentação acima.
  int iplot = 0; // É o índice do ponto do plot. Será

  char direct = 'd'; // Move o motor para a direita.

  Serial.println(selection);

  for ( int imed = 0; imed < tot_medicoes; imed++)
  {
    motorStep(direct, skip_steps); // Avança 5 passos.
    // Inicia a medição com médias
    medida = 0.0;
    for (int k = 0; k < N_medias; k++)
    {
      analogRead(CA); //Lixo
      medida = medida + analogRead(CA);  //Efetiva a leitura
      delay(1);
    }
    medida = medida / N_medias;
    vector[imed] = (int) medida; // Atualiza o elemento do array de médias com casting int.
    //    Serial.println(vector[imed]);

    // Calcula quantas vezes a medida atual deve ser plotada:
    // Como tenho o valor exato de iplot, esta rotina pode ser otimizada.
    // Mas, dado que isto funciona, deixarei como está.
    int contagem = 0;
    for (int j = 0; j < plotPoints; j++) {
      if (round(j * conversion) == imed) contagem++;
    }

    for (int j = 0; j < contagem; j++) {
      int x, y;
      if (selection == 't') {
        y = round(1023 * float(vector[imed]) / float(I0[imed]));
        yBox.write(round(y / 1023.0 * 100)); //Atualiza o y atual
      }
      else {
        y = vector[imed];
        yBox.write(y); //Atualiza o y atual
      }
      // O TrendCurve não plota pontos que sejam y=0. Isso significa que parte da curva pode ser cortada.
      // Como o y é de 0-1023, é só redefinir y=1.
      if (y<1) y=1;
      switch (currentChannel) {
        case 0:
          Lcm.writeTrendCurve0(y);
          break;
        case 1:
          Lcm.writeTrendCurve1(y);
          break;
        case 2:
          Lcm.writeTrendCurve2(y);
          break;
      }
      x = map(iplot, 0, plotPoints - 1, lambda_min, lambda_max);
      xBox.write(x); //Atualiza o x atual
      //      Serial.println(iplot);
      iplot++;
    }
  }

  delay(2000);

  // Retorna ao ponto inicial
  direct = 'e';
  motorStep(direct, tot_medicoes * skip_steps);
  Serial.println(0);
}


void Localizar()
{
  /*
   * Função para localizar o comprimento de onda de referência de 540 nm do cursor
   */
  int control = 0;
  char direct;

  // Inicialmente, o cursor moverá para a esquerda até atingir
  // a chave/o sensor mecânico.
  direct = 'e';
  while (control == 0)
  {
    motorStep(direct, 1);
    control = digitalRead(Chave_1);
  }

  // Agora, o cursor moverá para a direita até atingir o sensor óptico
  direct = 'd';
  control = 1;
  while (control == 1)
  {
    motorStep(direct, 1);
    control = digitalRead(Sensor_2);
  }

}

void NovaPosicao(int lambda_min)
{
  /*
   * Função que atualiza a posição atual do cursor
   */
  int passos;
  char direct;
  if (lambda_min > posicao) // Se o cursor estiver à esquerda do ponto mínimo, mova à direita
  {
    passos = (int)(lambda_min - posicao) * rate;
    direct = 'd';
  }
  else // Contrário de antes
  {
    passos = (int)(posicao - lambda_min) * rate;
    direct = 'e';
  }
  motorStep(direct, passos);

  // Atualiza a posição atual do leitor
  posicao = lambda_min;
}

void motorStep(char direct, int passos) {
  /*
     Esta rotina automatiza os passos dados pelo motor.

     direc: direção; 'e' ou 'd'.
     passos: total de passos deslocados na direção.
  */
  // Definição da direção:
  if (direct == 'e') {
    digitalWrite(Sentido_1, HIGH); // Direção esquerda
  }
  else {
    digitalWrite(Sentido_1, LOW); // Direção direita
  }

  // Avanço de passos:
  for (int i = 0; i < passos; i++) {
    digitalWrite(Pulso_1, HIGH);
    delay(10);
    digitalWrite(Pulso_1, LOW);
    delay(10);
  }
}
