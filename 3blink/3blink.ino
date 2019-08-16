
int switchState = 0;
int state = 1;

void setup() {
  pinMode(2, INPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(9600);
  }

void loop() {
  int newSwitchState = digitalRead(2);

  if (newSwitchState != switchState) {
    switchState = newSwitchState;
    if (switchState == LOW) {
      state++;
      digitalWrite(3, (state % 3 == 1 ? HIGH : LOW));
      digitalWrite(4, (state % 3 == 2 ? HIGH : LOW));
      digitalWrite(5, (state % 3 == 0 ? HIGH : LOW));
      Serial.print("state: ");
      Serial.print(state);
      Serial.print("\n");
    }
  }
  delay(250);
}
