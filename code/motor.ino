// ========================== Controle Motores =========================== //
//
// @name: motor.ino
// @date: 24/02/16
// @desc: Funções para controle do motor
//
//

#define ESQUERDO 1
#define DIREITO 2

Int 
  In1 = 4,
  In2 = 5,
  In3 = 6,
  In4 = 7,
  EnA = 10,
  EnB = 11,
  VelEsq = 0,
  VelDir = 0,
  VelMin = 120,
  VelMax = 245;

void Mover(int direcao, int velocidade) {
  velocidade = abs(velocidade);
  if (direcao == 1) {
    digitalWrite(In1, HIGH);
    digitalWrite(In2, LOW);       // Anda pra
    digitalWrite(In3, LOW);       // Frente
    digitalWrite(In4, HIGH);
  } else if (direcao == 2) {
    digitalWrite(In1, LOW);
    digitalWrite(In2, HIGH);      // Anda pra
    digitalWrite(In3, HIGH);      // Trás
    digitalWrite(In4, LOW);
  } else if (direcao == 0) {
    morreu();
  }
    if (velocidade > VelMax) {
      velocidade = VelMax;
    } else if (velocidade < VelMin) {
      velocidade = VelMin;
    }
    analogWrite(EnA, velocidade);
    analogWrite(EnB, velocidade);
} 
void morreu() {
    digitalWrite(In1, HIGH);
    digitalWrite(In2, HIGH);       // FREIA TUDO
    digitalWrite(In3, HIGH);
    digitalWrite(In4, HIGH);
}
