
void displayMessage(String msg, unsigned int msg_time, bool returnToId){
  /*
   * Escreve uma mensagem geral display por tempo definido (em ms).
   * Pode retornar ou não à tela inicial antes de esta função ser acionada.
   */
  byte currentId = Lcm.readPicId(); // Armazena o ID da tela inicial
  Lcm.changePicId(11); // Muda para a tela das mensagens
  lcmMsg.write(msg); // Escreve a mensagem no objeto TextDisplay lcmMsg
  delay(msg_time); // Trava o programa na tela pelo tempo determinado
  if(returnToId) Lcm.changePicId(currentId); // Se exigido, retornar à tela inicial
  
}

void clearArray(int vector[650]){
  for(int i=0;i<650;i++){
    vector[i]=0;
  }
}

void printVect(int vector[650]){
  /*
   * Função simples que envia para a serial os I0(lambda) ou I(lambda), dependendo do vetor utilizado na função.
   */
  for(int i=0;i<tot_medicoes;i++){
    Serial.println(vector[i]);
  }
}

void printTransm(){
  /*
   * Função simples que envia para a Serial todos os T(lambda)
   */
  for(int i=0;i<tot_medicoes;i++){
    Serial.println(float(I[i])/I0[i]);
  }
}
