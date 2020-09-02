  #include <EEPROM.h>
   
  char bar[13];
  bool code[114];
  
  String auxString;
  //Identificador do tipo .pgm
  char magicNumber[3] = "P2";
  //Guarda a Descrição do arquivo .pgm
  char fileName[7] = "ean13_";//6
  //Guarda a quantidade de vezes que o programa foi executado em sequencia
  int fileNumber = 0;

  static void s_print_bool_arr(bool* arr, int n)
  {
      int i;
      
      for (i = 0; i < n; i++) {
          Serial.print((int)arr[i]);
      }
      Serial.print("\r\n");
  }

  //Facilita a separação das sequências aceitas pelo código de barras do lado esquerdo
  enum parity_e {
      PARITY_ODD = 0,
      PARITY_EVEN = 1
  };
  
  //Separação inicial antes do código de barras
  static bool quiet_zone[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; //9
  //Representa o início do código de barras
  static bool lead_trailer[] = {1, 0, 1}; //3
  //Representa a divisão entre o lado esquerdo e direito do código de barras
  static bool separator[] = {0, 1, 0, 1, 0}; //5
  
  //Combinações possiveis do lado esquerdo do código de barras
  static bool modules_odd_left[10][7] = {
      {0, 0, 0, 1, 1, 0, 1}, {0, 0, 1, 1, 0, 0, 1}, {0, 0, 1, 0, 0, 1, 1}, {0, 1, 1, 1, 1, 0, 1},
      {0, 1, 0, 0, 0, 1, 1}, {0, 1, 1, 0, 0, 0, 1}, {0, 1, 0, 1, 1, 1, 1}, {0, 1, 1, 1, 0, 1, 1},
      {0, 1, 1, 0, 1, 1, 1}, {0, 0, 0, 1, 0, 1, 1}//70
  };
  
  //Outras combinações possiveis do lado esquerdo do código de barras
  static bool modules_even_left[10][7] = {
      {0, 1, 0, 0, 1, 1, 1}, {0, 1, 1, 0, 0, 1, 1}, {0, 0, 1, 1, 0, 1, 1}, {0, 1, 0, 0, 0, 0, 1},
      {0, 0, 1, 1, 1, 0, 1}, {0, 1, 1, 1, 0, 0, 1}, {0, 0, 0, 0, 1, 0, 1}, {0, 0, 1, 0, 0, 0, 1},
      {0, 0, 0, 1, 0, 0, 1}, {0, 0, 1, 0, 1, 1, 1}//70
  };
  
  //Combinações possiveis do lado esquerdo do código de barras
  static bool modules_right[10][7] = {
      {1, 1, 1, 0, 0, 1, 0}, {1, 1, 0, 0, 1, 1, 0}, {1, 1, 0, 1, 1, 0, 0}, {1, 0, 0, 0, 0, 1, 0},
      {1, 0, 1, 1, 1, 0, 0}, {1, 0, 0, 1, 1, 1, 0}, {1, 0, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0},
      {1, 0, 0, 1, 0, 0, 0}, {1, 1, 1, 0, 1, 0, 0}//70
  };
  
  //Tipos de combinações permitidas para o lado esquerdo do código de barras
  static bool parities[10][5] = {
      {0, 0, 0, 0, 0}, {0, 1, 0, 1, 1}, {0, 1, 1, 0, 1}, {0, 1, 1, 1, 0}, {1, 0, 0, 1, 1},
      {1, 1, 0, 0, 1}, {1, 1, 1, 0, 0}, {1, 0, 1, 0, 1}, {1, 0, 1, 1, 0}, {1, 1, 0, 1, 0}//50
  };
  
  //Escreve os bits dentro do array para o código de barras
  static void write_n(bool *dest, bool *src, int n)
  {
      int i;
      for (i = 0; i < n; i++) {
          dest[i] = src[i];
      }
  }

  //Constroi o código de barras em bits
  int EAN13_build(char *data, bool *code)
  {
      int index = 0;
      int num = 0;
      int start = 0;
      int i;
      
      //Constroi um array contendo o código de barras a partir do
      //array com os dados de entrada.
      for(i=0; i<12; i++) {
          if((data[i] < 0x30) || (data[i] > 0x39)) {
              return -1;
          }
      }
      
      //Preenche o array com zeros (limpeza)
      for (i = 0; i < 113; i++) {
          code[i] = 0;
      }
      
      //Criando a zona branca do código de barras
      write_n(code + index, quiet_zone, sizeof(quiet_zone));
      index += sizeof(quiet_zone);
      
      //Criando o entrada/início do código de barras
      write_n(code + index, lead_trailer, sizeof(lead_trailer));
      index += sizeof(lead_trailer);
      
      //Guardando o primeiro dígito (indica o tipo do código)
      start = data[0] - 0x30;//truque para converter char em int
      
      
      num = data[1] - 0x30;
      //Escreve os bits do primeiro digito no array código de barras
      write_n(code + index, modules_odd_left[num], 7);
      index += 7;
      
      //Escreve os demais dígitos do lado esquerdo do array código de barras
      for (i = 2; i < 7; i++) {
          num = data[i] - 0x30;
          if (parities[start][i - 2] == 0) {
              write_n(code + index, modules_odd_left[num], 7);
          } else {
              write_n(code + index, modules_even_left[num], 7);
          }
          index += 7;
      }
      
      //Cria a separação central no array código de barras
      write_n(code + index, separator, sizeof(separator));
      index += sizeof(separator);
      
      //Escreve os demais dígitos do lado direito do array código de barras
      for (i = 7; i < 12; i++) {
          num = data[i] - 0x30;
          write_n(code + index, modules_right[num], 7);
          index += 7;
      }
      
      //Calcula o dígito verificador
      int odds = 0;
      int evens = 0;
      for (int i = 0; i < 12; i++) {
          if (i % 2 == 0) {
              evens += data[i] - 0x30;
          } else {
              odds += data[i] - 0x30;
          }
      }
      
      //Escreve os bits correspondentes ao dígito verificador
      int checksum = 10 - (((odds * 3) + evens) % 10);
      write_n(code + index, modules_right[checksum], 7);
      index += 7;
      
      //Criando o fim/saída do código de barras
      write_n(code + index, lead_trailer, sizeof(lead_trailer));
      index += sizeof(lead_trailer);
      
      //Criando a zona branca final do código de barras
      write_n(code + index, quiet_zone, sizeof(quiet_zone));
      index += sizeof(quiet_zone);
      
      return 0;
  }
    void printCode(bool *code){
      for (int j = 0; j < 113; j++){
        int auxInt = (int)code[j];
        auxInt = (auxInt+1)%2;
        Serial.print(auxInt);
        Serial.print(' ');
      }
      Serial.print("\r\n");
    }

    void cleanMatrix(bool matrixData[10][113]){
    for (int j = 0; j < 10; j++){
      for (int i = 0; i<113; i++){
        matrixData[j][i] = 0;
      }
    }
  }

  void saveInEEPROM(bool *code){
    int addr = 0;
    for (int j = 0; j < 9; j++){
        for (int i = 0; i<113; i++){
          EEPROM.write(addr, code[i]);
          addr++;
        }
     }
  }

  void printFromEEPROM(void){
    int addr = 0;
    for (int j = 0; j < 9; j++){
        for (int i = 0; i<113; i++){
          int auxInt = (int)EEPROM.read(addr);
          auxInt = (auxInt+1)%2;
          Serial.print(auxInt);
          Serial.print(' ');
          addr++;
        }
        Serial.print("\r\n");
     }
  }
  
  void createMatrixData(bool matrixData[10][113], bool *code ){
    for (int j = 0; j < 10; j++){
        for (int i = 0; i<113; i++){
            matrixData[j][i] = code[i];
        }
     }
  }

  void printFromMatrix(bool matrixData[10][113]){
    //Cria o header para uma imagem tipo .pgm
    //writePgmHeader(ean13);
    
    for (int j = 0; j < 10; j++){
      for (int i = 0; i<113; i++){
          
      //inverte o número pois na imagem 1 é branco e 0 é preto
      int auxInt = (int)matrixData[j][i];
      auxInt = (auxInt+1)%2;

      Serial.print(auxInt);
      Serial.print(' ');
    }
    
    Serial.print("\r\n");
    }
  }

  void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
  }
  
  void loop() {
    //Gauarda entrada do programa
    bool matrixData[10][113];
    cleanMatrix(matrixData);
 
    Serial.write("Insira um código numérico: (12 digitos)\n");
    
    while (Serial.available() == 0){}; 
    auxString = Serial.readString();

    auxString.toCharArray(bar, 13);

    EAN13_build(bar, code);

    //s_print_bool_arr(code, 113);

    createMatrixData(matrixData, code);
    saveInEEPROM(code);

    Serial.print("Codigo recebido: ");
    Serial.println(bar);
    Serial.print("\r\n");
    Serial.println(magicNumber);
    Serial.print("# ");
    Serial.print(fileName);
    Serial.print(fileNumber);
    Serial.print(".pgm");
    Serial.print("\r\n");
    Serial.println("113 20");
    Serial.println("1");
    
    printCode(code);
    printFromMatrix(matrixData);
    printFromEEPROM();

    Serial.print("\r\n");
    fileNumber++;
  }

  