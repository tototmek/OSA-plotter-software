#define LED_INDICATOR 12
#define X_LIMIT_SWITCH 8
#define Y_LIMIT_SWITCH 9
#define Z_LIMIT_SWITCH 10
#define X_DIRECTION 2
#define X_STEP 3
#define Y_DIRECTION 4
#define Y_STEP 5
#define Z_DIRECTION 6
#define Z_STEP 7
#define PING_RESPONSE_STR "OSA-plotter"
#define HOMING_STEP_PERIOD 666
#define HOME_POSITION_X 500
#define HOME_POSITION_Y 2000
#define HOME_POSITION_Z 1000
#define X_MAX 31200
#define Y_MAX 28000
#define Z_MAX 16000
#define COMMAND_BUFFER_SIZE 216

int commandBuffer[COMMAND_BUFFER_SIZE][3];
int bufferSize = 0;

int MOVING_STEP_PERIOD = 333;

long xPos = 0;
long yPos = 0;
long zPos = 0;

void setup() {
  pinMode(LED_INDICATOR, OUTPUT);
  pinMode(X_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(Y_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(Z_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(X_DIRECTION, OUTPUT);
  pinMode(X_STEP, OUTPUT);
  pinMode(Y_DIRECTION, OUTPUT);
  pinMode(Y_STEP, OUTPUT);
  pinMode(Z_DIRECTION, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  Serial.begin(9600);
  wait_for_connection();
}

void wait_for_connection() {
  while(true) {
    if (Serial.available() > 0) {
      if (Serial.read() == 1) {
        Serial.println("ok");
        digitalWrite(LED_INDICATOR, HIGH);
        return;
      } else {
        Serial.println(PING_RESPONSE_STR);
      }
    }
  }
}

void loop() {
  // Wait for commands
  if (Serial.available() > 0) {
      switch(Serial.read()) {
        case 0: 
          Serial.println(PING_RESPONSE_STR);
          break;
        case 1:
          Serial.println("E1");
          break;
        case 2:
          home_procedure();
          Serial.println("ok");
          break;
        case 16:
          sendPosition();
          break;
        case 17:
          setPosition();
          break;
        case 18:
          receiveBuffer();
          executeBuffer();
          break;
        case 19:
          setMovingStepPeriod();
          break;
        default:
          Serial.println("E0");
          break;
      }
    }
}

void home_procedure() {
  bool xDone = false;
  bool yDone =  false;
  bool zDone = false;
  bool xState = false;
  bool yState = false;
  bool zState = false;
  digitalWrite(X_DIRECTION, LOW);
  digitalWrite(Y_DIRECTION, HIGH);
  digitalWrite(Z_DIRECTION, HIGH);
  while ((!xDone) || (!yDone) || (!zDone)) {
      if (!xDone) {
          xState = !xState;
          digitalWrite(X_STEP, xState);
          if (!digitalRead(X_LIMIT_SWITCH)) {
            xDone = true;
            }
          }
      if (!yDone) {
          yState = !yState;
          digitalWrite(Y_STEP, yState);
          if (!digitalRead(Y_LIMIT_SWITCH)) {
            yDone = true;
        }
      }
      if (!zDone) {
          zState = !zState;
          digitalWrite(Z_STEP, zState);
          if (!digitalRead(Z_LIMIT_SWITCH)) {
            zDone = true;
        }
      }
      delayMicroseconds(HOMING_STEP_PERIOD);
    }
  xPos = 0;
  yPos = 0;
  zPos = 0;
  delay(333);
  moveUniformly(HOME_POSITION_X, HOME_POSITION_Y, HOME_POSITION_Z);
}

void sendPosition() {
  Serial.println(String(xPos) + '/' + yPos + '/' + zPos);
}

void setPosition() {
  int dX = Serial.parseInt();
  int dY = Serial.parseInt();
  int dZ = Serial.parseInt();
  moveUniformly(dX, dY, dZ);
  Serial.println("ok");
}

void receiveBuffer() {
  int n = Serial.parseInt(); // Number of commands in the buffer
  if (n > COMMAND_BUFFER_SIZE) {
      Serial.println("E3");
      return;
    }
  for (int i=0; i < n; i++) {
    int dX = Serial.parseInt();
    int dY = Serial.parseInt();
    int dZ = Serial.parseInt();
    commandBuffer[i][0] = dX;
    commandBuffer[i][1] = dY;
    commandBuffer[i][2] = dZ;
  }
  bufferSize = n;
}

void executeBuffer() {
  for (int i=0; i < bufferSize; i++) {
        moveUniformly(commandBuffer[i][0], commandBuffer[i][1], commandBuffer[i][2]);
    }
  bufferSize = 0;
  Serial.println("ok");
}

void setMovingStepPeriod() {
  MOVING_STEP_PERIOD = Serial.parseInt();
  Serial.println("ok");
}

void moveUniformly(int dX, int dY, int dZ) {
  long dist = sqrt(float(pow(dX, 2) + pow(dY, 2) + pow(dZ, 2)));
  long pX = dist * MOVING_STEP_PERIOD / (dX != 0 ? abs(dX) : 1);
  long pY = dist * MOVING_STEP_PERIOD / (dY != 0 ? abs(dY) : 1);
  long pZ = dist * MOVING_STEP_PERIOD / (dZ != 0 ? abs(dZ) : 1);
  if (xPos +  dX > X_MAX || yPos + dY > Y_MAX || zPos + dZ > Z_MAX || xPos + dX < 0 || yPos + dY < 0 || zPos + dZ < 0) {
      Serial.println("E2");
      return;
    }
  xPos += dX;
  yPos += dY;
  zPos += dZ;
  if (dX < 0) {
    dX = -dX;
    digitalWrite(X_DIRECTION, LOW);
  } else {
    digitalWrite(X_DIRECTION, HIGH);
  }
  if (dY < 0) {
    dY = -dY;
    digitalWrite(Y_DIRECTION, HIGH);
  } else {
    digitalWrite(Y_DIRECTION, LOW);
  }
  if (dZ < 0) {
    dZ = -dZ;
    digitalWrite(Z_DIRECTION, HIGH);
  } else {
    digitalWrite(Z_DIRECTION, LOW);
  }
  bool xState = false;
  bool yState = false;
  bool zState =  false;
  bool xDone = dX == 0;
  bool yDone = dY == 0;
  bool zDone = dZ == 0;
  unsigned long current_time = micros();
  unsigned long xNextTime = current_time + pX;
  unsigned long yNextTime = current_time + pY;
  unsigned long zNextTime = current_time + pZ;
  while (!(xDone && yDone && zDone)) {
      current_time = micros();
      if (!xDone) {
            if (xNextTime < current_time) {
                xNextTime += pX;
                xState = !xState;
                digitalWrite(X_STEP, xState);
                dX -= 1;
                if (dX == 0) {
                    xDone = true;    
                }
            }
        }
        if (!yDone) {
            if (yNextTime < current_time) {
                yNextTime += pY;
                yState = !yState;
                digitalWrite(Y_STEP, yState);
                dY -= 1;
                if (dY == 0) {
                    yDone = true;    
                }
            }
        }
        if (!zDone) {
            if (zNextTime < current_time) {
                zNextTime += pZ;
                zState = !zState;
                digitalWrite(Z_STEP, zState);
                dZ -= 1;
                if (dZ == 0) {
                    zDone = true;    
                }
            }
        }
    }
}
