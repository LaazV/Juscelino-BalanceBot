#include <PID_v1.h>

double Setpoint, Input, Output;
double Kp = 1, Ki = 0, Kd = 0.4;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
const int sampleRate = 1;
int direcao, erroPID;

void PID_setup()
{
  Setpoint = 127;
  myPID.SetOutputLimits(120, 255);
  myPID.SetMode(AUTOMATIC);
  Serial.println("PID Iniciado\n");
  
}

int PID_loop(float angulo) {

  // angulo < 0 -> anda pra frente
  // angulo >= 0, anda pra trás
  angulo < 0 ? direcao = 1 : direcao = 2;

  Input = map(angulo, -40, 40, 0, 255);

  Input > Setpoint ? Input = Input - (2*(Input-Setpoint)): Input = Input;
    
  myPID.Compute();
  Serial.print("\nPID: ");
  Serial.print(Output);
  Serial.print("\n");

  Mover(direcao, Output);

}
