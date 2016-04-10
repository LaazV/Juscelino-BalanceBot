// I2Cdev library collection - MPU6050 I2C device class, 6-axis MotionApps 2.0 implementation
// Based on InvenSense MPU-6050 register map document rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 5/20/2013 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib
//
// Changelog:
//     ... - ongoing debug release
/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2012 Jeff Rowberg
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Wire.h>
//#include "MPU6050.h" // não é necessário se estiver usando o MPU6050_6Axis_MotionApps20.h

MPU6050 mpu;


/* =========================================================================
   Este programa precisa que o pino de interrupção do MPU6050 esteja conectado
   ao pino digital 2 do arduino (Interrupt #0).
 * ========================================================================= */

#define LED_PIN 13 // LED integrado do Arduino

bool 
    blinkState = false,
    // Variáveis de controle e status da MPU
    dmpReady = false;  // verdadeiro se a inicialização do DMP aconteceu com sucesso

uint8_t 
    mpuIntStatus,   // guarda o status do interrupt
    devStatus,    // retorna o status depois de cada operação (0 = success, !0 = error)
    fifoBuffer[64]; // FIFO storage buffer

uint16_t 
    packetSize,   // tamanho esperado do pacote do DMP (42 bytes é o padrão)
    fifoCount;     // conntagem de todos os bytes guardados no FIFO

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                 Variáveis de Orientação e Movimento                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

Quaternion q;
VectorFloat gravity;

float 
    ypr[3],
    yall,
    pitch,
    roll,
    GYRO_FACTOR,  // escala do giroscópio
    ACCEL_FACTOR; // escala do acelerômetro

const float 
    RADIANS_TO_DEGREES = 57.2958;


// ================================================================
// ===               Rotina de Detecção de Interrupção          ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high

void dmpDataReady() {
  mpuInterrupt = true;
}

void MPU_setup() {


  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();

  // initialize serial communication
  Serial.begin(57600);
  while (!Serial); // wait for Leonardo enumeration, others continue immediately

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  
  // Calibra o sensor
  mpu.setXAccelOffset(1694);
  mpu.setYAccelOffset(-954);
  mpu.setZAccelOffset(5665);
  mpu.setXGyroOffset(118);
  mpu.setYGyroOffset(45);
  mpu.setZGyroOffset(-28);


  // verify connection
  Serial.println(F("Testando conexoes com dispositivo"));
  Serial.println(mpu.testConnection() ? F("Conectado MPU6050") : F("Conexao MPU6050 falhou"));

  /*  No waiting necessary for this version
  // wait for ready
  Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  while (Serial.available() && Serial.read()); // empty buffer
  while (!Serial.available());                 // wait for data
  while (Serial.available() && Serial.read()); // empty buffer again
  */

  // load and configure the DMP
  Serial.println(F("Inicializando o DMP..."));
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    Serial.println(F("Ligando o DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Habilitando deteccao de interrupcao (Arduino external interrupt 0)..."));
    attachInterrupt(0, dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP Pronto! Esperando a primeira interrupcao..."));
    dmpReady = true;

    // Set the full scale range of the gyro
    uint8_t FS_SEL = 0;
    //mpu.setFullScaleGyroRange(FS_SEL);

    // get default full scale value of gyro - may have changed from default
    // function call returns values between 0 and 3
    uint8_t READ_FS_SEL = mpu.getFullScaleGyroRange();
    Serial.print("FS_SEL = ");
    Serial.println(READ_FS_SEL);
    GYRO_FACTOR = 131.0 / (FS_SEL + 1);


    // get default full scale value of accelerometer - may not be default value.
    // Accelerometer scale factor doesn't reall matter as it divides out
    uint8_t READ_AFS_SEL = mpu.getFullScaleAccelRange();
    Serial.print("AFS_SEL = ");
    Serial.println(READ_AFS_SEL);
    //ACCEL_FACTOR = 16384.0/(AFS_SEL + 1);

    // Set the full scale range of the accelerometer
    //uint8_t AFS_SEL = 0;
    //mpu.setFullScaleAccelRange(AFS_SEL);

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("Falha na inicializacao do DMP (codigo "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  // configure LED for output
  pinMode(LED_PIN, OUTPUT);

  // get calibration values for sensors
  
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void MPU_loop() {
  if (!dmpReady) return;
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // busca a contagem do FIFO atual
  fifoCount = mpu.getFIFOCount();

  // Checa por overflow no FIFO
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // Reseta para continuar
    mpu.resetFIFO();
    Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & 0x02) {
    // Espera até a quantidade de dados disponibilizada pela FIFO seja igual ao tamanho do pacote esperado.
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
    
    mpu.getFIFOBytes(fifoBuffer, packetSize);
    fifoCount -= packetSize;
    // Obtem os angulos Yall, Pitch, Roll do Buffer
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    roll = -ypr[1] * RADIANS_TO_DEGREES;
    yall = ypr[0] * RADIANS_TO_DEGREES;
    pitch = ypr[2] * RADIANS_TO_DEGREES;

    Serial.print("\tYall: ");
    Serial.print(yall);
    //Serial.print("\tPitch: ");
    //Serial.print(pitch);
    Serial.print("\tRoll: ");
    Serial.print(roll);
    Serial.println("");

    if (roll > 40 || roll < -40) {
      if (roll < 65 || roll > -65) {
        morreu();
      } else {
      morreu();
      }
      //Serial.print("Acima de 30 graus");
    } else {
      PID_loop(roll);
    }
    


    // pisca o LED para indicar atividade
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
  }
}
