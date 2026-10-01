#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Scalable cell count ----------
// Change NUM_CELLS only. Cells 1-4 are the real pots; extra cells are simulated.
#define NUM_CELLS 4
#define REAL_CELLS 4
#if NUM_CELLS < REAL_CELLS
#error "NUM_CELLS must be at least 4"
#endif

// Inputs
const int CELL_PIN[REAL_CELLS] = {34, 35, 32, 33};

// Outputs
#define RED_LED 2
#define GREEN_LED 4
#define YELLOW_LED 5

#define BUZZER 18
#define RELAY 19

// ---------- BMS data (one place for all battery info) ----------
struct BmsData {
  float v[NUM_CELLS];      // cell voltages
  int   pct[NUM_CELLS];    // cell charge %
  int   weakest, strongest;
  int   avgPct;            // pack SoC %
  float imbalance;         // strongest - weakest (V)
  float threshold;         // adaptive imbalance limit (V)
  char  trend;             // 'U' rising, 'D' falling, 'S' steady
};
BmsData bms;

String status;

// Function to read voltage
float readVoltage(int pin) {
  int raw = analogRead(pin);
  return (raw / 4095.0) * 3.3;
}

// Convert voltage -> percentage
int getPercent(float v) {
  return constrain(map(v * 100, 0, 330, 0, 100), 0, 100);
}

// Real pot for cells 1-4, simulated for the rest
float readCell(int i) {
  if (i < REAL_CELLS) return readVoltage(CELL_PIN[i]);
  float avg = 0;
  for (int k = 0; k < REAL_CELLS; k++) avg += readVoltage(CELL_PIN[k]);
  avg /= REAL_CELLS;
  return constrain(avg + ((i % 3) - 1) * 0.05, 0.0, 3.3);
}

// ---------- BMS analysis (reusable) ----------
void analyzeBms() {
  static bool first = true;
  float prevImb = bms.imbalance;
  float sum = 0;

  bms.weakest = 0;
  bms.strongest = 0;

  for (int i = 0; i < NUM_CELLS; i++) {
    bms.v[i] = readCell(i);
    bms.pct[i] = getPercent(bms.v[i]);
    sum += bms.pct[i];
    if (bms.v[i] < bms.v[bms.weakest]) bms.weakest = i;
    if (bms.v[i] > bms.v[bms.strongest]) bms.strongest = i;
  }

  bms.avgPct = sum / NUM_CELLS;
  bms.imbalance = bms.v[bms.strongest] - bms.v[bms.weakest];

  // Adaptive threshold: 0.10 V when full -> 0.30 V when empty
  bms.threshold = 0.10 + 0.20 * (100 - bms.avgPct) / 100.0;

  // Trend of imbalance (0.02 V dead-band)
  if (first) { bms.trend = 'S'; first = false; }
  else {
    float d = bms.imbalance - prevImb;
    if (d > 0.02) bms.trend = 'U';
    else if (d < -0.02) bms.trend = 'D';
    else bms.trend = 'S';
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  lcd.init();
  lcd.backlight();

  digitalWrite(RELAY, LOW); // ACTIVE LOW -> ON

  Serial.println("=== MULTI CELL BMS STARTED ===");
}

void loop() {

  // Read + analyze
  analyzeBms();

  // Reset outputs
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(BUZZER, LOW);

  // Check limits
  bool anyLow = false, allHigh = true;
  for (int i = 0; i < NUM_CELLS; i++) {
    if (bms.pct[i] < 30) anyLow = true;
    if (bms.pct[i] <= 80) allHigh = false;
  }

  // Decision logic
  if (anyLow) {
    status = "WEAK CELL";
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(RELAY, HIGH); // CUT OFF
  }
  else if (allHigh) {
    status = "HIGH VOLT";
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(RELAY, HIGH); // CUT OFF
  }
  else if (bms.imbalance > bms.threshold) {
    status = "IMBALANCE";
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    // cut off only if the imbalance is getting worse
    digitalWrite(RELAY, bms.trend == 'U' ? HIGH : LOW);
  }
  else {
    status = "NORMAL";
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RELAY, LOW); // CONNECT
  }

  // Serial Debug
  Serial.println("-----------");
  for (int i = 0; i < NUM_CELLS; i++) {
    Serial.print("C"); Serial.print(i + 1); Serial.print(": ");
    Serial.print(bms.v[i], 2); Serial.print("V ");
  }
  Serial.println();
  Serial.print("SoC: "); Serial.print(bms.avgPct);
  Serial.print("%  Weakest: C"); Serial.print(bms.weakest + 1);
  Serial.print("  Strongest: C"); Serial.println(bms.strongest + 1);
  Serial.print("Imbalance: "); Serial.print(bms.imbalance, 2);
  Serial.print("V  Limit: "); Serial.print(bms.threshold, 2);
  Serial.print("V  Trend: ");
  Serial.println(bms.trend == 'U' ? "INCREASING" : bms.trend == 'D' ? "DECREASING" : "STEADY");
  Serial.println(status);

  // LCD Display (three pages, switches every second)
  static int page = 0;
  page = (page + 1) % 3;
  lcd.clear();

  if (page == 0) {
    lcd.setCursor(0, 0);
    lcd.print("C1:");
    lcd.print(bms.v[0], 1);
    lcd.print(" C2:");
    lcd.print(bms.v[1], 1);

    lcd.setCursor(0, 1);
    lcd.print("C3:");
    lcd.print(bms.v[2], 1);
    lcd.print(" C4:");
    lcd.print(bms.v[3], 1);
  } else if (page == 1) {
    lcd.setCursor(0, 0);
    lcd.print("W:C");
    lcd.print(bms.weakest + 1);
    lcd.print(" S:C");
    lcd.print(bms.strongest + 1);
    lcd.print(" T:");
    lcd.print(bms.trend == 'U' ? "^" : bms.trend == 'D' ? "v" : "=");

    lcd.setCursor(0, 1);
    lcd.print("Imb:");
    lcd.print(bms.imbalance, 2);
    lcd.print(" Th:");
    lcd.print(bms.threshold, 2);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(status);

    lcd.setCursor(0, 1);
    lcd.print("SoC:");
    lcd.print(bms.avgPct);
    lcd.print("%");
  }

  delay(1000);
}
