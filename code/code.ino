float angulo;

void setup() {

	// Executa o setup do angulo.ino	
	MPU_setup(); 
	PID_setup();
  
}

void loop() {

	MPU_loop();
  	
}

