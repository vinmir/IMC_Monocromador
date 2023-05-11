void SDbegin() {
  // Inicia o SD
  if (!SD.begin(SD_CS_PIN)) {
    // Se o SD não for encontrado, trava o programa.
    Lcm.changePicId(8);
    while (true);
  }
}

void sdSave(String fname) {
  /*
     Salva os dados do T atual em um cartão SD.
     A string tem no máximo 16 caracteres. Um array de char precisa de 16+1 elementos por causa do '\0'.
  */
  char str[17];
  fname.toCharArray(str, 17); // Converte a string em um array de char
  SD.remove(str); // Usa o array de char para deletar o arquivo
  arquivo = SD.open(str, FILE_WRITE); // Abre o arquivo do zero
  if (!arquivo) {
    Lcm.changePicId(8); // Se o arquivo falhar, trocar para a tela de erro
    delay(2000);
  }
  else {
    // Arquivo abriu.
    for (int i = 0; i < tot_medicoes; i++) {
      // x: comprimento de onda da medição
      // y: transmissão de cada comprimento de onda
      // Resolução: 2 casas decimais
      // Gera um arquivo .csv
      arquivo.print(lambda_min+i*float(lambda_max-lambda_min)/(tot_medicoes-1),2);
      arquivo.print(',');
      arquivo.println(float(I[i])/float(I0[i]),2);
    }
    arquivo.close(); // Fecha o arquivo
    Lcm.changePicId(9); // Troca para a tela de salvamento bem-sucedido
    delay(2000);
  }
}
