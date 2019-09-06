
#define LS_PIN_O  0
#define LS_PIN_1  2
#define LS_PIN_2  32
#define LS_PIN_3  12
#define LS_PIN_4  14


void play_sound(int number) {
  digitalWrite(LS_PIN_O, LOW);
  digitalWrite(LS_PIN_1, LOW);
  digitalWrite(LS_PIN_2, LOW);
  digitalWrite(LS_PIN_3, LOW);
  digitalWrite(LS_PIN_4, LOW);

  switch (number)
  {
    case 1:
      digitalWrite(LS_PIN_O, HIGH);
      break;
    case 2:
      digitalWrite(LS_PIN_1, HIGH);
      break;
    case 3:
      digitalWrite(LS_PIN_O, HIGH);
      digitalWrite(LS_PIN_1, HIGH);
      break;
  }

  delay(100);

  digitalWrite(LS_PIN_O, LOW);
  digitalWrite(LS_PIN_1, LOW);
  digitalWrite(LS_PIN_2, LOW);
  digitalWrite(LS_PIN_3, LOW);
  digitalWrite(LS_PIN_4, LOW);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LS_PIN_O, OUTPUT);
  pinMode(LS_PIN_1, OUTPUT);
  pinMode(LS_PIN_2, OUTPUT);
  pinMode(LS_PIN_3, OUTPUT);
  pinMode(LS_PIN_4, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  static int nr = 1;

  play_sound(nr++);

  if (nr > 3) nr = 1;

  delay(2000);
  

}
