void adjustLambda(bool displace) {
  /*
   * Função que ajusta a faixa de valores de lambda nos quais a medição ocorrerá.
   * Se displace=true, o cursor moverá após alterar lambda_min e lambda_max.
   */
  Lcm.changePicId(5); // Troca para a tela de ajuste de lambda
  while (true) {
    lcmLambdaMinDisplay.write(lambda_min); // Escreve os limites atuais de lambda
    lcmLambdaMaxDisplay.write(lambda_max);

    // Lê se o usuário quer retornar ao menu inicial
    if (lambdaReturn.available()) {
      byte buttonVal = lambdaReturn.getData();
      if (buttonVal == 1) break;
    }

    // Lê o lambda_min do usuário
    if (lcmLambdaMinInput.available()) {
      lambda_min = lcmLambdaMinInput.getData();
      if (lambda_min > lambda_max) lambda_min = lambda_max - 100;
      if (lambda_min < 300) lambda_min = 300;
    }

    // Lê o lambda_max do usuário
    if (lcmLambdaMaxInput.available()) {
      lambda_max = lcmLambdaMaxInput.getData();
      if (lambda_max < lambda_min) lambda_max = lambda_min + 100;
      if (lambda_max > 1000) lambda_max = 1000;
    }

  }
  if (displace) {
    // Desloca o cursor até a posição correta
    Lcm.changePicId(10); // Tela de carregamento
    NovaPosicao(lambda_min);
  }
  // Limpa todos os plots
  // Obrigatório: como lambda_min mudou, I0 deve mudar (e, por efeito cascata, T).
  cleanPlot(0, 0, 0);
  cleanPlot(0, 0, 1);
  cleanPlot(0, 0, 2);
  
}

void saveToSDScreen() {
  /*
   * Esta é a tela onde o usuário escolhe o nome do arquivo a ser salvo no SD.
   */
  Lcm.changePicId(4); // Tela de salvamento no SD
  String fname = "trans.csv"; // Nome padrão do arquivo a ser salvo
  while (true) {
    fileNameDisplay.write(fname); // Escreve o nome atual do arquivo
    if (SDbutton.available()) {
      byte buttonVal = SDbutton.getData();
      if (buttonVal == 0) break; // Usuário volta ao menu principal
      if (buttonVal == 1) {
        // Usuário opta por salvar o arquivo
        sdSave(fname);
        break;
      }
    }
    // Recebe o nome do arquivo imposto pelo usuário
    if (fileNameInput.available()) {
      fname = ""; // Limpa totalmente a String
      while (fileNameInput.available()) {
        // Adiciona um por um cada caractere recebido à string
        fname += (char)fileNameInput.getData();
      }
      // Se a string não encerrar com o formato correto, adicione um .csv
      if (!fname.endsWith(".csv")) {
        fname += ".csv";
      }
    }
  }
  Lcm.changePicId(1); // Retorna à tela principal do plot
}

void lockStart() {
  /*
     Rotina inicial:
      0. Inicia o SD. Se o SD nem mesmo for detectado, o programa não executa.
      1. Move o cursor ao ponto de referência 540 nm
      2. Move o cursor até o lambda_min
      3. Muda a tela do display para a tela do plot
  */
  while (true) {
    if (startButton.available()) {
      if (startButton.getData() == 1) {
        SDbegin(); // Inicia o cartão SD
        displayMessage("Abra o serial monitor p/ usar funcoes extras!", 2000,false);
        adjustLambda(false); // Requisita o comprimento de onda inicial
        Lcm.changePicId(10);
        Localizar();                    //FUNÇÃO PARA ENCONTRAR O PONTO DE REFERENCIA
        delay(1000);
        NovaPosicao(lambda_min);        //VAI PARA O COMPRIMENTO DE ONDA INICIAL
        break;
      }
    }
  }
  Lcm.changePicId(1); // Muda para a tela principal
  cleanPlot(0, 0, 0); // Limpa todos os canais do plot
  cleanPlot(0, 0, 1);
  cleanPlot(0, 0, 2);
}
