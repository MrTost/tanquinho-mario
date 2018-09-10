#include <LiquidCrystal.h>

//melodia do MARIO THEME
const int marioFrequ[] = {660,660,660,510,660,770,380,510,380,320,440,480,450,430,380,660,760,860,700,760,660,520,580,480,510,380,320,440,480,450,430,380,660,760,860,700,760,660,520,580,480};
const int marioDurac[] = {100,100,100,100,100,100,100,100,100,100,100, 80,100,100,100, 80, 50,100, 80, 50, 80, 80, 80, 80,100,100,100,100, 80,100,100,100, 80, 50,100, 80, 50, 80, 80, 80, 80};
const int marioPausa[] = {150,300,300,100,300,550,575,450,400,500,300,330,150,300,200,200,150,300,150,350,300,150,150,500,450,400,500,300,330,150,300,200,200,150,300,150,350,300,150,150,500};

// Pinos
LiquidCrystal lcd(6, 7, 5, 4, 3, 2);
const int pinBtUp      = 12;
const int pinBtDown    = 8;
const int pinBtLeft    = 10;
const int pinBtRight   = 11;
const int pinBtSet     = 9;
const int pinBuzzer    = 13;
const int pinBackLight = A0;
const int pinBomba     = A1;
const int pinMotor     = A2;
const int pinTorneira  = A3;
const int pinNivel     = A7;

// CONSTANTES
const int displayDelay   = 3000;  // tempo de trocar de info no display
const int iddleDelay     = 30000; // tempo de inatividade para depois desligar o display
const int btDelay        = 300;
const long enxagueBaterT  = 120000; // tempo que ira bater dentro do enxague
const long esvaziarDelayI = 10000; // tempo que a bomba ficara desligada para melhor leitura do nivel 
const long esvaziarDelayF = 30000; // tempo que a bomba ira ficar ligada depois que atingir o nivel zero

// Sensor de Nivel
int nivel        = 0; //variavel global com o nivel
const int nivel0 = 35;
const int nivel1 = 160;
const int nivel2 = 230;
const int nivel3 = 290;
const int nivel4 = 350;
const int nivel5 = 400;
const int nivelTol = 0.05;
const int nivelBuffer = 300; //numero de leituras para retornar o valor

boolean alterandoNivel = false;
boolean batendo        = false;
boolean demolho        = false;
boolean enxagueVazio   = false;
boolean enxagueCheio   = false;
boolean enxagueBatido  = false;

// Tempos para delay
unsigned long esvaziarDelayIT = 0;
unsigned long esvaziarDelayFT = 0;
unsigned long btPressT        = 0;

// Tempo de inicio de cada estagio
unsigned long cicloT0       = 0;
unsigned long encherT0      = 0;
unsigned long baterT0       = 0;
unsigned long molhoT0       = 0;
unsigned long enxagueT0     = 0;
unsigned long esvaziarT0    = 0;
unsigned long pauseT0       = 0; // inicio da pausa
unsigned long lastActiveT0  = 0; // inicio de inatividade
unsigned long displayT0     = 0;

// Tempo estimado de cada estagio
unsigned long cicloT    = 0;
unsigned long encherT   = 0;
unsigned long baterT    = 0;
unsigned long molhoT    = 0;
unsigned long enxagueT  = 0;
unsigned long esvaziarT = 0;

// BOTOES
int btUp     = HIGH;
int btDown   = HIGH;
int btLeft   = HIGH;
int btRight  = HIGH;
int btSet    = HIGH;
int lcdLight = HIGH;

// VARIAVEIS DE INPUT DE USUARIO
int inTempo       = 5;
int inNivel       = 1;
boolean inMolho   = false;
boolean inEnxague = false;
boolean inReuso   = false;

// 0 = Trabalho (Iniciar, Pausa, Retomar ou Cancelar)
// 1 = Tempo (5 a 20) minutos
// 2 = Nivel (1 a 5)
// 3 = Molho (sim - nao)
// 4 = Enxaguar (sim - nao)
// 5 = Reuso agua (sim - nao)
int funcao = 0;

// 1 = enchendo
// 2 = batendo
// 3 = molho
// 4 = enxaguando
// 5 = esvaziando
int estagio = 1;

boolean isWorking = false;
boolean inRetomar = true;

void setup() {
  // Bomba
  pinMode(pinBomba, OUTPUT);
  digitalWrite(pinBomba, HIGH);
  // Motor
  pinMode(pinMotor, OUTPUT);
  digitalWrite(pinMotor, LOW);
  // Torneira
  pinMode(pinTorneira, OUTPUT);
  digitalWrite(pinTorneira, HIGH);
  // Sensor Nivel
  pinMode(pinNivel, INPUT);
  // Buzzer
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, LOW);
  // Botoes
  pinMode(pinBtUp, INPUT_PULLUP);
  pinMode(pinBtDown, INPUT_PULLUP);
  pinMode(pinBtLeft, INPUT_PULLUP);
  pinMode(pinBtRight, INPUT_PULLUP);
  pinMode(pinBtSet, INPUT_PULLUP);
  // LCD
  lcd.begin(16, 2);
  pinMode(pinBackLight, OUTPUT);

  //Serial.begin(9600);

  loadTemposCiclo();

  turnOnDisplay();
  updateDisplay(true);
}

void loop() {
  buttons();
  turnOffIfIddle();

  if (isWorking) {
    lastActiveT0 = millis();
    boolean nextEstagio = false;
    
    if (estagio == 1) {
      nextEstagio = encher(inNivel);
    } else if (estagio == 2) {
      nextEstagio = bater(baterT);
    } else if (estagio == 3) {
      nextEstagio = molho(molhoT);
    } else if (estagio == 4) {
      nextEstagio = enxague(inNivel);
    } else if (estagio == 5) {
      nextEstagio = esvaziar(inReuso);
    }
    
    if (nextEstagio) {
      //Serial.print("------------------------------------------------------------------------- \n");

      if (estagio == 2) {
        if (inMolho) {
          estagio = 3; // molho
        } else if (inEnxague) {
          estagio = 4; // enxague
        } else if (!inReuso) {
          estagio = 5; // esvaziar
        } else {
          estagio = 6; // fim
        }
      } else if (estagio == 3) {
        if (inEnxague) {
          estagio = 4; // enxague
        } else if (!inReuso) {
          estagio = 5; // esvaziar
        } else {
          estagio = 6; // fim
        }
      } else {
        estagio++;
      }   
      
      updateDisplay(true);
    }
  
    if (estagio > 5) {
      playMario();
      resetVariaveis(true);
      loadTemposCiclo();
      updateDisplay(true);
    } else {
      updateDisplay(false);
    }
    
  }

  //int nivel = getNivel();
  //Serial.print("Nivel: ");
  //Serial.print(nivel);
  //Serial.print(" \n");
}

void buttons() {
  btUp    = digitalRead(pinBtUp);
  btDown  = digitalRead(pinBtDown);
  btLeft  = digitalRead(pinBtLeft);
  btRight = digitalRead(pinBtRight);
  btSet   = digitalRead(pinBtSet);

  if (btPressT == 0 && ((btUp == LOW && btDown == HIGH && btLeft == HIGH && btRight == HIGH && btSet == HIGH) ||
                        (btUp == HIGH && btDown == LOW && btLeft == HIGH && btRight == HIGH && btSet == HIGH) ||
                        (btUp == HIGH && btDown == HIGH && btLeft == LOW && btRight == HIGH && btSet == HIGH) ||
                        (btUp == HIGH && btDown == HIGH && btLeft == HIGH && btRight == LOW && btSet == HIGH) ||
                        (btUp == HIGH && btDown == HIGH && btLeft == HIGH && btRight == HIGH && btSet == LOW))) {
    btPressT = millis();
    lastActiveT0 = millis();
    
    if (lcdLight == LOW) {
      tone(pinBuzzer,1800,150);
      turnOnDisplay();
    } else {

      if (btUp == LOW && !isWorking && pauseT0 == 0) {
        tone(pinBuzzer,1800,150);
        funcao = ((funcao + 1) > 5 ? 0 : (funcao + 1));
      } else if (btDown == LOW && !isWorking && pauseT0 == 0) {
        tone(pinBuzzer,1800,150);
        funcao = ((funcao - 1) < 0 ? 5 : (funcao - 1));
      } else if (btLeft == LOW && (!isWorking || pauseT0 > 0)) {
        tone(pinBuzzer,1800,150);
        setParam(false);
        if (!isWorking) {
          loadTemposCiclo();
        }
      } else if (btRight == LOW && (!isWorking || pauseT0 > 0)) {
        tone(pinBuzzer,1800,150);
        setParam(true);
        if (!isWorking) {
          loadTemposCiclo();
        }
      } else if (btSet == LOW) {
        if (!isWorking && funcao == 0 && pauseT0 == 0) {
          // iniciar ciclo customizado
          tone(pinBuzzer,1800,150);
          loadTemposCiclo();
          cicloT0 = millis();
          pauseT0 = 0;
          isWorking = true;
        } else if (isWorking && pauseT0 == 0) {
          // pausar
          //Serial.print("Pausar \n");
          tone(pinBuzzer,1800,150);
          desligar();
          pauseT0        = millis();
          isWorking      = false;
          alterandoNivel = false;
          batendo        = false;
          demolho        = false;
        } else if (!isWorking && pauseT0 > 0) {
          if (inRetomar) {
            // retomar
            //Serial.print("Retomar \n");
            tone(pinBuzzer,1800,150);
            isWorking = true;
            
            //ajustar tempos afetados pela pausa
            unsigned long pauseL = millis();
            pauseL -= pauseT0;

            if (cicloT0 > 0) {
              cicloT += pauseL;
            }
            if (encherT0 > 0) {
              encherT += pauseL;
            }
            if (baterT0 > 0) {
              baterT += pauseL;
            }
            if (molhoT0 > 0) {
              molhoT += pauseL;
            }
            if (enxagueT0 > 0) {
              enxagueT += pauseL;
            }
            if (esvaziarT0 > 0) {
              esvaziarT += pauseL;
            }
          } else {
            // cancelar
            //Serial.print("Cancelar \n");
            tone(pinBuzzer,1800,150);
            resetVariaveis(true);
            loadTemposCiclo();
          }
          pauseT0 = 0;
        }
      }

      updateDisplay(true);
    }
  }

  // habilitar repeticao quando botao ficar pressionado
  if (millis() - btPressT >= btDelay && btPressT > 0) {
    btPressT = 0; //liberar para novo comando
  }
}
void setParam(boolean up) {
  if (pauseT0 > 0) {
    inRetomar = (inRetomar ? false : true);   
  }else if (funcao == 1) {
    if (up) {
      inTempo = ((inTempo + 1) > 25 ? 5 : (inTempo + 1));
    } else {
      inTempo = ((inTempo - 1) < 5 ? 25 : (inTempo - 1));
    }
  } else if (funcao == 2) {
    if (up) {
      inNivel = ((inNivel + 1) > 5 ? 1 : (inNivel + 1));
    } else {
      inNivel = ((inNivel - 1) < 1 ? 5 : (inNivel - 1));
    }
  } else if (funcao == 3) {
    inMolho = (inMolho ? false : true);
  } else if (funcao == 4) {
    inEnxague = (inEnxague ? false : true);
  } else if (funcao == 5) {
    inReuso = (inReuso ? false : true);
  }
}
void resetVariaveis(boolean setActive) {
  alterandoNivel = false;
  batendo        = false;
  demolho        = false;
  enxagueVazio   = false;
  enxagueCheio   = false;
  enxagueBatido  = false;
  
  // Tempos para delay
  esvaziarDelayIT = 0;
  esvaziarDelayFT = 0;
  //btPressT = 0; //nao resetar controle de botao
  
  // Tempo de inicio de cada estagio
  cicloT0       = 0;
  encherT0      = 0;
  baterT0       = 0;
  molhoT0       = 0;
  enxagueT0     = 0;
  esvaziarT0    = 0;
  pauseT0       = 0; // inicio da pausa
  
  if (setActive) {
    lastActiveT0  = millis(); // inicio de inatividade
  }  
  
  // Tempo estimado de cada estagio
  cicloT    = 0;
  encherT   = 0;
  baterT    = 0;
  molhoT    = 0;
  enxagueT  = 0;
  esvaziarT = 0;

  nivel     = 0;
  
  // VARIAVEIS DE INPUT DE USUARIO
  inTempo = 5;
  inNivel = 1;
  inMolho = false;
  inEnxague = false;
  inReuso = false;
  
  // 0 = Trabalho (Iniciar, Pausa, Retomar ou Cancelar)
  // 1 = Tempo (5 a 20) minutos
  // 2 = Nivel (1 a 5)
  // 3 = Molho (sim - nao)
  // 4 = Enxaguar (sim - nao)
  // 5 = Reuso agua (sim - nao)
  funcao = 0;
  
  isWorking = false;
  inRetomar = true;
  
  // 1 = enchendo
  // 2 = batendo
  // 3 = molho
  // 4 = enxaguando
  // 5 = esvaziando
  estagio = 1;
}

// -------------------------------------------------------------------------------------------
// Funcoes de Display ------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void turnOffIfIddle() {
  // Desligar o display depois de um tempo de inatividade
  if (!isWorking && lcdLight == HIGH && pauseT0 == 0) {
    unsigned long iddleDelayL = millis();
    iddleDelayL -= lastActiveT0;

    if (iddleDelayL >= iddleDelay) {
      lcdLight = LOW;
      lcd.noDisplay();
      digitalWrite(pinBackLight, lcdLight);
      resetVariaveis(false);
    }
  }
}
void turnOnDisplay() {
  lcdLight = HIGH;
  lcd.display();
  digitalWrite(pinBackLight, lcdLight);
  loadTemposCiclo();
  updateDisplay(true);
}
void updateDisplay(boolean clean) {
  if (clean) {
    lcd.clear();
  } else {
    lcd.setCursor(0,0);
  }
  
  if (funcao == 0) {
    if (!isWorking && pauseT0 == 0) {
      lcd.print("Iniciar");
      lcd.setCursor(0,1);
      lcd.print("Estimado");
      printTime(false, cicloT);
    } else if (isWorking && pauseT0 == 0) {
      
      unsigned long lapsed = 0;
      
      if (estagio == 1) {
        lcd.print("Enchendo   ");
        lapsed = encherT - (millis() - encherT0);
        printTime(true, lapsed);        
      } else if (estagio == 2) {
        lcd.print("Batendo    ");
        lapsed = baterT - (millis() - baterT0);
        printTime(true, lapsed);
      } else if (estagio == 3) {
        lcd.print("Repouso    ");
        lapsed = molhoT - (millis() - molhoT0);
        printTime(true, lapsed);
      } else if (estagio == 4) {
        lcd.print("Enxague    ");
        lapsed = enxagueT - (millis() - enxagueT0);
        printTime(true, lapsed);
      } else if (estagio == 5) {
        lcd.print("Esvaziando ");
        lapsed = esvaziarT - (millis() - esvaziarT0);
        printTime(true, lapsed);
      }
      lcd.setCursor(0,1);
      
      if (displayT0 == 0) {
        displayT0 = millis();
      }

      unsigned long displayL = millis();
      displayL -= displayT0;

      if (displayL >= 0 && displayL <= displayDelay) {
        lcd.print("Termino    ");
        lapsed = cicloT - (millis() - cicloT0);
        printTime(false, lapsed);
      } else if (displayL > displayDelay && displayL <= (displayDelay * 2)) {
        if (estagio == 1 || estagio == 5 || (estagio == 4 && alterandoNivel)) {
          lcd.print("Nivel      ");
          lcd.setCursor(11,1);
          lcd.print(nivel);
          if (nivel < 10) {
            lcd.print("    ");
          } else if (nivel < 100) {
            lcd.print("   ");
          } else if (nivel < 1000) {
            lcd.print("  ");
          } else if (nivel < 10000) {
            lcd.print(" ");
          }
        } else {
          displayT0 = 0;
        }
      } else {
        displayT0 = 0;
      }
    } else if (pauseT0 > 0) {
      lcd.print("Pause");
      lcd.setCursor(0,1);
      lcd.print((inRetomar ? "Retomar Ciclo" : "Cancelar Ciclo"));
    }
  } else if (funcao == 1) {
    lcd.print("Tempo de Operacao");
    lcd.setCursor(0,1);
    lcd.print(inTempo);
  } else if (funcao == 2) {
    lcd.print("Nivel da Agua");
    lcd.setCursor(0,1);
    lcd.print(inNivel);
  } else if (funcao == 3) {
    lcd.print("Molho");
    lcd.setCursor(0,1);
    lcd.print((inMolho ? "Sim" : "Nao"));
  } else if (funcao == 4) {
    lcd.print("Enxaguar");
    lcd.setCursor(0,1);
    lcd.print((inEnxague ? "Sim" : "Nao"));
  } else if (funcao == 5) {
    lcd.print("Reusar a Agua");
    lcd.setCursor(0,1);
    lcd.print((inReuso ? "Sim" : "Nao"));
  }
  
}
void printTime(boolean firstLine, unsigned long lapsed) {
  if (firstLine) {
    lcd.setCursor(11,0);
  } else {
    lcd.setCursor(11,1);
  }
  
  int lapMin = (lapsed/1000/60);
  int lapSeg = ((lapsed/1000)%60);

  if (lapMin < 10) {
     lcd.print("0");
  }
  lcd.print(lapMin);
  lcd.print(":");
  
  if (lapSeg < 10) {
     lcd.print("0");
  }
  lcd.print(lapSeg);
}

// -------------------------------------------------------------------------------------------
// Funcoes de lavagem ------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void desligar() {
  digitalWrite(pinTorneira, HIGH);
  digitalWrite(pinBomba, HIGH);
  digitalWrite(pinMotor, LOW);
}
int getNivelConst(int nivelS) {
  if (nivelS == 0) {
    return nivel0;
  } else if (nivelS == 1) {
    return nivel1;
  } else if (nivelS == 2) {
    return nivel2;
  } else if (nivelS == 3) {
    return nivel3;
  } else if (nivelS == 4) {
    return nivel4;
  } else if (nivelS == 5) {
    return nivel5;
  } else {
    return nivel0;
  }
}
int getNivel() {
  float nivelS = 0;
  for (int i=0; i <= nivelBuffer; i++){
    nivelS += analogRead(pinNivel);
  }
  nivelS /= nivelBuffer;
  return (int) nivelS;
}
boolean alterarNivel(int newNivel) {
  nivel = getNivel();
  
  if (newNivel == nivel0) {
    // tratamento especial para no nivel0, já que este deve ter um delay de ligar e desligar

    if (esvaziarDelayIT == 0) {
      //Serial.print("Iniciando delay I de esvaziar \n");
      esvaziarDelayIT = millis();
    }

    unsigned long esvaziarDelayIL = millis();
    esvaziarDelayIL -= esvaziarDelayIT;

    if (!alterandoNivel && esvaziarDelayIL <= esvaziarDelayI) {
      return false;
    } else {
      if (!alterandoNivel) {
        digitalWrite(pinTorneira, HIGH);
        digitalWrite(pinBomba, LOW);
        digitalWrite(pinMotor, LOW);
        alterandoNivel = true;
        //Serial.print("alterar nivel - ");
        //Serial.print(nivel);
        //Serial.print(" -> ");
        //Serial.print(newNivel);
        //Serial.print(" => esvaziando \n");
      }
  
      if (nivel <= nivel0) {
        if (esvaziarDelayFT == 0) {
          //Serial.print("Iniciando delay F de esvaziar \n");
          esvaziarDelayFT = millis();
        }

        unsigned long esvaziarDelayFL = millis();
        esvaziarDelayFL -= esvaziarDelayFT;
  
        if (esvaziarDelayFL <= esvaziarDelayF) {
          return false;
        } else {
          esvaziarDelayIT = 0;
          esvaziarDelayFT = 0;
          if (alterandoNivel) {
            desligar();
            alterandoNivel = false;
            //Serial.print("alterar nivel - ");
            //Serial.print(nivel);
            //Serial.print(" -> ");
            //Serial.print(newNivel);
            //Serial.print(" => nivel calibrado \n");
          }
          return true;
        }
      } else {
        return false;
      }
    }
  } else {
    if (nivel < newNivel) {
      // encher
      if (!alterandoNivel) {
        digitalWrite(pinTorneira, LOW);
        digitalWrite(pinBomba, HIGH);
        digitalWrite(pinMotor, LOW);
        alterandoNivel = true;
        //Serial.print("alterar nivel - ");
        //Serial.print(nivel);
        //Serial.print(" -> ");
        //Serial.print(newNivel);
        //Serial.print(" => enchendo \n");
      }    
      return false; 
    } else if (nivel > newNivel) {
      // esvaziar
      if (!alterandoNivel) {
        digitalWrite(pinTorneira, HIGH);
        digitalWrite(pinBomba, LOW);
        digitalWrite(pinMotor, LOW);
        alterandoNivel = true;
        //Serial.print("alterar nivel - ");
        //Serial.print(nivel);
        //Serial.print(" -> ");
        //Serial.print(newNivel);
        //Serial.print(" => esvaziando \n");
      }    
      return false;
    } else {
      // nivel ajustado
      if (alterandoNivel) {
        desligar();
        alterandoNivel = false;
        //Serial.print("alterar nivel - ");
        //Serial.print(nivel);
        //Serial.print(" -> ");
        //Serial.print(newNivel);
        //Serial.print(" => nivel calibrado \n");
      }    
      return true; 
    }
  }
}
boolean encher(int nivelS) {
  if (encherT0 == 0) {
    encherT0 = millis();
    //Serial.print("INI - Encher \n");
  }

  boolean isCheio = alterarNivel(getNivelConst(nivelS));

  if (isCheio) {
    encherT0 = 0;
    //Serial.print("FIM - Encher \n");
  }
  
  return isCheio;
}
boolean esvaziar(boolean isReuso) {
  if (esvaziarT0 == 0) {
    esvaziarT0 = millis();
    //Serial.print("INI - Esvaziar \n");
  }
  
  if (!isReuso) {
    boolean isVazio = alterarNivel(nivel0);

    if (isVazio) {
      esvaziarT0 = 0;
      //Serial.print("FIM - Esvaziar \n");
    }
  
    return isVazio;
  } else {
    esvaziarT0 = 0;
    //Serial.print("FIM - Esvaziar \n");
    return true;
  }
}
boolean bater(unsigned long tempoBater) {
  if (baterT0 == 0) {
    baterT0 = millis();
    //Serial.print("INI - Bater \n");
  }

  //TODO - criar delay para a agua parar de girar e ter uma leitura melhor do nivel
  unsigned long baterDelay = millis();
  baterDelay -= baterT0;

  if (baterDelay <= tempoBater) {
    if (!batendo) {
      digitalWrite(pinTorneira, HIGH);
      digitalWrite(pinBomba, HIGH);
      digitalWrite(pinMotor, HIGH);
      batendo = true;
    }
    return false;
  } else {
    if (batendo) {
      desligar();
      batendo = false;
    }
    baterT0 = 0;
    //Serial.print("FIM - Bater \n");
    return true;
  }
}
boolean molho(unsigned long tempoMolho) {
  if (!inMolho) {
    return true;
  }
  
  if (molhoT0 == 0) {
    molhoT0 = millis();
    //Serial.print("INI - Molho \n");
  }

  unsigned long molhoDelay = millis();
  molhoDelay -= molhoT0;

  if (molhoDelay <= tempoMolho) {
    if (!demolho) {
      desligar();
      demolho = true;
    }    
    return false;
  } else {
    if (demolho) {
      desligar();
    }
    molhoT0 = 0;
    //Serial.print("FIM - Molho \n");
    return true;
  }
}
boolean enxague(int nivelS) {
  if (!inEnxague) {
    return true;
  }
  
  if (enxagueT0 == 0) {
    enxagueT0 = millis();
    //Serial.print("INI - Enxague \n");
  }

  if (!enxagueVazio) {
    enxagueVazio = esvaziar(false);
    return false;
  } else {
    if (!enxagueCheio) {
      enxagueCheio = encher(nivelS);
      return false;
    } else {
      if (!enxagueBatido) {
        enxagueBatido = bater(enxagueBaterT);
        return false;
      } else {
        enxagueVazio   = false;
        enxagueCheio   = false;
        enxagueBatido  = false;
        enxagueT0 = 0;
        //Serial.print("FIM - Enxague \n");
        return true;
      }
    }
  }
}
void playMario() {
  lcd.clear();
  lcd.print("Fim do Ciclo");
  
  tone(pinBuzzer,1800,1000);
  delay(1000);
  
  for (int nota = 0; nota < 41; nota++) {
    tone(pinBuzzer, marioFrequ[nota], marioDurac[nota]);
    delay(marioPausa[nota]);
  }
}

// -------------------------------------------------------------------------------------------
// Tempos e estimativas ----------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

unsigned long loadTemposCiclo() {
  nivel = getNivel();

  encherT = getTempoAjusteNivel(nivel, getNivelConst(inNivel));
  
  if (!inReuso) {
    esvaziarT = getTempoAjusteNivel(getNivelConst(inNivel), nivel0);
  } else {
    esvaziarT = 0;
  }

  if (inEnxague) {
    enxagueT = 0;
    enxagueT += getTempoAjusteNivel(getNivelConst(inNivel), nivel0); //esvaziar
    enxagueT += getTempoAjusteNivel(nivel, getNivelConst(inNivel)); //encher
    enxagueT += enxagueBaterT; //bater
  } else {
    enxagueT = 0;
  }

  cicloT = inTempo * 60;
  cicloT *= 1000;

  molhoT = (inMolho ? (cicloT / 2) : 0); //1min a 5min
  baterT = cicloT - molhoT;

  cicloT += enxagueT;
  cicloT += encherT;
  cicloT += esvaziarT;
  
  cicloT *= 1.02; //mais 2% de tolerancia na estimativa do tempo
}
unsigned long getTempoAjusteNivel(int oldNivel, int newNivel) {
  unsigned long ajusteNivelT = 0;
  
  if (oldNivel < newNivel) { // ENCHER
    //Nivel 1
    if ( (newNivel >= nivel1) && (oldNivel < (nivel1 - (nivel1 * nivelTol))) ) {
      ajusteNivelT += 280;
    }
    //Nivel 2
    if ( (newNivel >= nivel2) && (oldNivel < (nivel2 - (nivel2 * nivelTol))) ) {
      ajusteNivelT += 175;
    }
    //Nivel 3
    if ( (newNivel >= nivel3) && (oldNivel < (nivel3 - (nivel3 * nivelTol))) ) {
      ajusteNivelT += 181;
    }
    //Nivel 4
    if ( (newNivel >= nivel4) && (oldNivel < (nivel4 - (nivel4 * nivelTol))) ) {
      ajusteNivelT += 195;
    }
    //Nivel 5
    if ( (newNivel >= nivel5) && (oldNivel < (nivel5 - (nivel5 * nivelTol))) ) {
      ajusteNivelT += 189;
    }
  } else if (oldNivel > newNivel) { // ESVAZIAR
    //Nivel 0
    if ( (newNivel <= nivel0) && (oldNivel > (nivel0 + (nivel0 * nivelTol))) ) {
      ajusteNivelT += 180;
      ajusteNivelT += esvaziarDelayI / 1000; //mais tempo que a bomba fica ligada depois de atingir o nivel zero
      ajusteNivelT += esvaziarDelayF / 1000;
    }
    //Nivel 1
    if ( (newNivel <= nivel1) && (oldNivel > (nivel1 + (nivel1 * nivelTol))) ) {
      ajusteNivelT += 45;
    }
    //Nivel 2
    if ( (newNivel <= nivel2) && (oldNivel > (nivel2 + (nivel2 * nivelTol))) ) {
      ajusteNivelT += 45;
    }
    //Nivel 3
    if ( (newNivel <= nivel3) && (oldNivel > (nivel3 + (nivel3 * nivelTol))) ) {
      ajusteNivelT += 44;
    }
    //Nivel 4
    if ( (newNivel <= nivel4) && (oldNivel > (nivel4 + (nivel4 * nivelTol))) ) {
      ajusteNivelT += 38;
    }
  }

  ajusteNivelT *= 1000;

  return ajusteNivelT;
}

