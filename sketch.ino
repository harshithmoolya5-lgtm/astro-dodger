#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Servo gauge;

const int HORZ = A0, SW = 2, GREEN = 5, RED = 6, BUZ = 8, SERVO_PIN = 9;
const byte N = 4;                 // number of asteroids

int ax[N], ay[N];
int shipX = 60;
byte lives = 3;
int score = 0;
int fall = 2;                     // fall speed
bool playing = false;
unsigned long greenOff = 0, redOff = 0, lastFrame = 0;

void resetRock(byte i) {
  ax[i] = random(0, 120);
  ay[i] = -random(8, 70);
}

void newGame() {
  lives = 3; score = 0; fall = 2; shipX = 60;
  for (byte i = 0; i < N; i++) resetRock(i);
  gauge.write(0);
  playing = true;
}

void showText(const char* big, const char* small) {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 8);  oled.println(big);
  oled.setTextSize(1);
  oled.setCursor(0, 40); oled.println(small);
  oled.display();
}

void gameOver() {
  playing = false;
  tone(BUZ, 400, 200); delay(250);
  tone(BUZ, 300, 200); delay(250);
  tone(BUZ, 200, 400); delay(450);
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);  oled.println("GAME OVER");
  oled.setTextSize(1);
  oled.setCursor(0, 28); oled.print("Score: "); oled.println(score);
  oled.setCursor(0, 48); oled.println("Press SW to retry");
  oled.display();
}

void setup() {
  pinMode(SW, INPUT_PULLUP);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(BUZ, OUTPUT);
  gauge.attach(SERVO_PIN);
  gauge.write(0);
  randomSeed(analogRead(A3));
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);
  showText("DODGER", "Press SW to start");
}

void loop() {
  unsigned long now = millis();

  // LED auto-off (non-blocking)
  if (greenOff && now > greenOff) { digitalWrite(GREEN, LOW); greenOff = 0; }
  if (redOff && now > redOff)     { digitalWrite(RED, LOW);   redOff = 0; }

  if (!playing) {
    if (digitalRead(SW) == LOW) { delay(200); newGame(); }
    return;
  }

  if (now - lastFrame < 30) return;   // ~33 FPS
  lastFrame = now;

  // Move ship
  int x = analogRead(HORZ);
  if (x < 400) shipX += 4;
  if (x > 600) shipX -= 4;
  shipX = constrain(shipX, 0, 116);

  // Update asteroids
  for (byte i = 0; i < N; i++) {
    ay[i] += fall;

    // Hit check (ship is 12 wide, at y = 56)
    if (ay[i] + 8 >= 56 && ay[i] <= 62 && ax[i] + 8 > shipX && ax[i] < shipX + 12) {
      lives--;
      digitalWrite(RED, HIGH); redOff = now + 300;
      tone(BUZ, 150, 300);
      resetRock(i);
      if (lives == 0) { gameOver(); return; }
    }
    // Dodged
    else if (ay[i] > 64) {
      score++;
      digitalWrite(GREEN, HIGH); greenOff = now + 100;
      tone(BUZ, 1200, 60);
      if (score % 5 == 0 && fall < 6) fall++;       // level up
      gauge.write(constrain(map(score, 0, 30, 0, 180), 0, 180));
      resetRock(i);
    }
  }

  // Draw
  oled.clearDisplay();
  for (byte i = 0; i < N; i++) oled.fillRect(ax[i], ay[i], 8, 8, SSD1306_WHITE);
  oled.fillTriangle(shipX + 6, 54, shipX, 63, shipX + 12, 63, SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);   oled.print("S:"); oled.print(score);
  oled.setCursor(90, 0);  oled.print("Life:"); oled.print(lives);
  oled.display();
}