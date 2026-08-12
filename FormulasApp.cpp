#include "FormulasApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppManager.h"
#include "TextBox.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;

// Avoids relying on M_PI being defined by the toolchain's math.h.
constexpr double kPi = 3.14159265358979323846;

bool nearZero(double v) { return fabs(v) < 1e-9; }

const char* kCategoryNames[FormulasApp::kCategoryCount] = {
    "Everyday", "Finance", "Geometry", "Physics", "Health", "Electronic",
};

struct FormulaField {
  const char* label;
  bool isToggle;
  const char* toggleOff;  // shown when the field's value is 0
  const char* toggleOn;   // shown when the field's value is nonzero
};

struct FormulaDef {
  const char* name;
  int category;  // index into kCategoryNames
  int fieldCount;
  FormulaField fields[FormulasApp::kMaxFields];
  void (*compute)(const double* v, char* out, size_t outLen);
};

const char* bmiCategory(double bmi) {
  if (bmi < 18.5) return "Underweight";
  if (bmi < 25.0) return "Normal";
  if (bmi < 30.0) return "Overweight";
  return "Obese";
}

// ---- Everyday ----

void computeTipSplit(const double* v, char* out, size_t len) {
  double bill = v[0], pct = v[1], people = v[2];
  if (people < 1.0) people = 1.0;
  double tip = bill * pct / 100.0;
  double total = bill + tip;
  snprintf(out, len, "Tip: $%.2f\nTotal: $%.2f\nEach Pays: $%.2f", tip, total,
           total / people);
}

void computeDiscount(const double* v, char* out, size_t len) {
  double price = v[0], pct = v[1];
  double save = price * pct / 100.0;
  snprintf(out, len, "You Save: $%.2f\nFinal Price: $%.2f", save,
           price - save);
}

void computePercentChange(const double* v, char* out, size_t len) {
  double oldV = v[0], newV = v[1];
  if (nearZero(oldV)) {
    snprintf(out, len, "Old Value must be non-zero");
    return;
  }
  snprintf(out, len, "Change: %.2f%%", (newV - oldV) / oldV * 100.0);
}

void computeTripFuelCost(const double* v, char* out, size_t len) {
  double distance = v[0], mpg = v[1], pricePerGal = v[2];
  if (mpg <= 0.0) {
    snprintf(out, len, "Fuel Economy must be positive");
    return;
  }
  double gallons = distance / mpg;
  snprintf(out, len, "Gallons Used: %.2f\nFuel Cost: $%.2f", gallons,
           gallons * pricePerGal);
}

void computeUnitPrice(const double* v, char* out, size_t len) {
  double price = v[0], qty = v[1];
  if (qty <= 0.0) {
    snprintf(out, len, "Quantity must be positive");
    return;
  }
  snprintf(out, len, "Price per Unit: $%.4f", price / qty);
}

// ---- Finance ----

void computeSimpleInterest(const double* v, char* out, size_t len) {
  double p = v[0], r = v[1], t = v[2];
  double interest = p * r / 100.0 * t;
  snprintf(out, len, "Interest: $%.2f\nTotal: $%.2f", interest,
           p + interest);
}

void computeCompoundInterest(const double* v, char* out, size_t len) {
  double p = v[0], r = v[1], n = v[2], t = v[3];
  if (n < 1.0) n = 1.0;
  double amount = p * pow(1.0 + (r / 100.0) / n, n * t);
  snprintf(out, len, "Future Value: $%.2f\nInterest Earned: $%.2f", amount,
           amount - p);
}

void computeLoanPayment(const double* v, char* out, size_t len) {
  double principal = v[0], annualRate = v[1], years = v[2];
  double n = years * 12.0;
  if (n <= 0.0) {
    snprintf(out, len, "Term must be positive");
    return;
  }
  double r = annualRate / 100.0 / 12.0;
  double monthly;
  if (nearZero(r)) {
    monthly = principal / n;
  } else {
    double f = pow(1.0 + r, n);
    monthly = principal * r * f / (f - 1.0);
  }
  snprintf(out, len, "Monthly Payment: $%.2f\nTotal Paid: $%.2f", monthly,
           monthly * n);
}

void computeSalesTax(const double* v, char* out, size_t len) {
  double price = v[0], rate = v[1];
  double tax = price * rate / 100.0;
  snprintf(out, len, "Tax: $%.2f\nTotal: $%.2f", tax, price + tax);
}

void computeRoi(const double* v, char* out, size_t len) {
  double initial = v[0], finalVal = v[1];
  if (nearZero(initial)) {
    snprintf(out, len, "Initial Value must be non-zero");
    return;
  }
  double gain = finalVal - initial;
  snprintf(out, len, "Gain: $%.2f\nROI: %.2f%%", gain,
            gain / initial * 100.0);
}

// ---- Geometry ----

void computeCircle(const double* v, char* out, size_t len) {
  double r = v[0];
  snprintf(out, len, "Area: %.3f\nCircumference: %.3f", kPi * r * r,
           2.0 * kPi * r);
}

void computeRectangle(const double* v, char* out, size_t len) {
  double l = v[0], w = v[1];
  snprintf(out, len, "Area: %.3f\nPerimeter: %.3f", l * w, 2.0 * (l + w));
}

void computeTriangleArea(const double* v, char* out, size_t len) {
  double b = v[0], h = v[1];
  snprintf(out, len, "Area: %.3f", 0.5 * b * h);
}

void computeRightTriangle(const double* v, char* out, size_t len) {
  double a = v[0], b = v[1];
  snprintf(out, len, "Hypotenuse: %.3f", sqrt(a * a + b * b));
}

void computeSphere(const double* v, char* out, size_t len) {
  double r = v[0];
  double vol = (4.0 / 3.0) * kPi * r * r * r;
  double surf = 4.0 * kPi * r * r;
  snprintf(out, len, "Volume: %.3f\nSurface Area: %.3f", vol, surf);
}

void computeCylinder(const double* v, char* out, size_t len) {
  double r = v[0], h = v[1];
  double vol = kPi * r * r * h;
  double surf = 2.0 * kPi * r * (r + h);
  snprintf(out, len, "Volume: %.3f\nSurface Area: %.3f", vol, surf);
}

// ---- Physics ----

void computeSpeed(const double* v, char* out, size_t len) {
  double distance = v[0], time = v[1];
  if (time <= 0.0) {
    snprintf(out, len, "Time must be positive");
    return;
  }
  snprintf(out, len, "Speed: %.3f m/s", distance / time);
}

void computeForce(const double* v, char* out, size_t len) {
  double m = v[0], a = v[1];
  snprintf(out, len, "Force: %.3f N", m * a);
}

void computeKineticEnergy(const double* v, char* out, size_t len) {
  double m = v[0], vel = v[1];
  snprintf(out, len, "Kinetic Energy: %.3f J", 0.5 * m * vel * vel);
}

void computeWork(const double* v, char* out, size_t len) {
  double f = v[0], d = v[1];
  snprintf(out, len, "Work: %.3f J", f * d);
}

void computeAcceleration(const double* v, char* out, size_t len) {
  double vi = v[0], vf = v[1], t = v[2];
  if (t <= 0.0) {
    snprintf(out, len, "Time must be positive");
    return;
  }
  snprintf(out, len, "Acceleration: %.3f m/s^2", (vf - vi) / t);
}

void computeDensity(const double* v, char* out, size_t len) {
  double m = v[0], vol = v[1];
  if (vol <= 0.0) {
    snprintf(out, len, "Volume must be positive");
    return;
  }
  snprintf(out, len, "Density: %.3f kg/m^3", m / vol);
}

// ---- Health ----

void computeBmi(const double* v, char* out, size_t len) {
  double weightLb = v[0], heightIn = v[1];
  if (heightIn <= 0.0) {
    snprintf(out, len, "Height must be positive");
    return;
  }
  double bmi = 703.0 * weightLb / (heightIn * heightIn);
  snprintf(out, len, "BMI: %.1f (%s)", bmi, bmiCategory(bmi));
}

void computeBmr(const double* v, char* out, size_t len) {
  double weightLb = v[0], heightIn = v[1], age = v[2];
  bool male = v[3] != 0.0;
  double wkg = weightLb * 0.453592;
  double hcm = heightIn * 2.54;
  double bmr = 10.0 * wkg + 6.25 * hcm - 5.0 * age + (male ? 5.0 : -161.0);
  snprintf(out, len, "BMR: %.0f Calories/day", bmr);
}

void computeTdee(const double* v, char* out, size_t len) {
  double bmr = v[0], multiplier = v[1];
  if (multiplier <= 0.0) {
    snprintf(out, len, "Activity Multiplier must be positive");
    return;
  }
  snprintf(out, len, "Daily Calories: %.0f", bmr * multiplier);
}

void computeTargetHeartRate(const double* v, char* out, size_t len) {
  double age = v[0];
  double maxHr = 220.0 - age;
  snprintf(out, len, "Max HR: %.0f bpm\nTarget Zone: %.0f - %.0f bpm", maxHr,
           maxHr * 0.50, maxHr * 0.85);
}

void computeWaterIntake(const double* v, char* out, size_t len) {
  double weightLb = v[0];
  snprintf(out, len, "Recommended: %.0f oz/day", weightLb * 0.5);
}

// ---- Electronic ----

void computeOhmsVoltage(const double* v, char* out, size_t len) {
  double i = v[0], r = v[1];
  snprintf(out, len, "Voltage: %.3f V", i * r);
}

void computeOhmsPower(const double* v, char* out, size_t len) {
  double volt = v[0], i = v[1];
  snprintf(out, len, "Power: %.3f W", volt * i);
}

void computeResistorsSeries(const double* v, char* out, size_t len) {
  double r1 = v[0], r2 = v[1];
  snprintf(out, len, "Total Resistance: %.3f Ohms", r1 + r2);
}

void computeResistorsParallel(const double* v, char* out, size_t len) {
  double r1 = v[0], r2 = v[1];
  if (nearZero(r1 + r2)) {
    snprintf(out, len, "R1 + R2 must be non-zero");
    return;
  }
  snprintf(out, len, "Total Resistance: %.3f Ohms", (r1 * r2) / (r1 + r2));
}

void computeVoltageDivider(const double* v, char* out, size_t len) {
  double vin = v[0], r1 = v[1], r2 = v[2];
  if (nearZero(r1 + r2)) {
    snprintf(out, len, "R1 + R2 must be non-zero");
    return;
  }
  snprintf(out, len, "Vout: %.3f V", vin * r2 / (r1 + r2));
}

const FormulaDef kFormulas[] = {
    // Everyday
    {"Tip & Split",
     0,
     3,
     {{"Bill ($)", false, "", ""},
      {"Tip %", false, "", ""},
      {"People", false, "", ""}},
     computeTipSplit},
    {"Discount Price",
     0,
     2,
     {{"Original Price ($)", false, "", ""}, {"Discount %", false, "", ""}},
     computeDiscount},
    {"Percent Change",
     0,
     2,
     {{"Old Value", false, "", ""}, {"New Value", false, "", ""}},
     computePercentChange},
    {"Trip Fuel Cost",
     0,
     3,
     {{"Distance (mi)", false, "", ""},
      {"Fuel Economy (mpg)", false, "", ""},
      {"Price/Gallon ($)", false, "", ""}},
     computeTripFuelCost},
    {"Unit Price",
     0,
     2,
     {{"Total Price ($)", false, "", ""}, {"Quantity", false, "", ""}},
     computeUnitPrice},

    // Finance
    {"Simple Interest",
     1,
     3,
     {{"Principal ($)", false, "", ""},
      {"Rate (%/yr)", false, "", ""},
      {"Time (yrs)", false, "", ""}},
     computeSimpleInterest},
    {"Compound Interest",
     1,
     4,
     {{"Principal ($)", false, "", ""},
      {"Rate (%/yr)", false, "", ""},
      {"Times/Year", false, "", ""},
      {"Years", false, "", ""}},
     computeCompoundInterest},
    {"Loan Payment",
     1,
     3,
     {{"Loan Amount ($)", false, "", ""},
      {"Annual Rate (%)", false, "", ""},
      {"Term (Years)", false, "", ""}},
     computeLoanPayment},
    {"Sales Tax",
     1,
     2,
     {{"Price ($)", false, "", ""}, {"Tax Rate (%)", false, "", ""}},
     computeSalesTax},
    {"Return on Investment",
     1,
     2,
     {{"Initial Value ($)", false, "", ""}, {"Final Value ($)", false, "", ""}},
     computeRoi},

    // Geometry
    {"Circle", 2, 1, {{"Radius", false, "", ""}}, computeCircle},
    {"Rectangle",
     2,
     2,
     {{"Length", false, "", ""}, {"Width", false, "", ""}},
     computeRectangle},
    {"Triangle Area",
     2,
     2,
     {{"Base", false, "", ""}, {"Height", false, "", ""}},
     computeTriangleArea},
    {"Right Triangle",
     2,
     2,
     {{"Side A", false, "", ""}, {"Side B", false, "", ""}},
     computeRightTriangle},
    {"Sphere", 2, 1, {{"Radius", false, "", ""}}, computeSphere},
    {"Cylinder",
     2,
     2,
     {{"Radius", false, "", ""}, {"Height", false, "", ""}},
     computeCylinder},

    // Physics
    {"Speed",
     3,
     2,
     {{"Distance (m)", false, "", ""}, {"Time (s)", false, "", ""}},
     computeSpeed},
    {"Force (F=ma)",
     3,
     2,
     {{"Mass (kg)", false, "", ""}, {"Acceleration (m/s^2)", false, "", ""}},
     computeForce},
    {"Kinetic Energy",
     3,
     2,
     {{"Mass (kg)", false, "", ""}, {"Velocity (m/s)", false, "", ""}},
     computeKineticEnergy},
    {"Work",
     3,
     2,
     {{"Force (N)", false, "", ""}, {"Distance (m)", false, "", ""}},
     computeWork},
    {"Acceleration",
     3,
     3,
     {{"Initial Velocity (m/s)", false, "", ""},
      {"Final Velocity (m/s)", false, "", ""},
      {"Time (s)", false, "", ""}},
     computeAcceleration},
    {"Density",
     3,
     2,
     {{"Mass (kg)", false, "", ""}, {"Volume (m^3)", false, "", ""}},
     computeDensity},

    // Health
    {"BMI",
     4,
     2,
     {{"Weight (lb)", false, "", ""}, {"Height (in)", false, "", ""}},
     computeBmi},
    {"BMR",
     4,
     4,
     {{"Weight (lb)", false, "", ""},
      {"Height (in)", false, "", ""},
      {"Age (yrs)", false, "", ""},
      {"Sex", true, "Female", "Male"}},
     computeBmr},
    {"Daily Calorie Needs",
     4,
     2,
     {{"BMR (Cal/day)", false, "", ""},
      {"Activity x (1.2-1.9)", false, "", ""}},
     computeTdee},
    {"Target Heart Rate",
     4,
     1,
     {{"Age (yrs)", false, "", ""}},
     computeTargetHeartRate},
    {"Water Intake",
     4,
     1,
     {{"Weight (lb)", false, "", ""}},
     computeWaterIntake},

    // Electronic
    {"Ohm's Law (Voltage)",
     5,
     2,
     {{"Current (A)", false, "", ""}, {"Resistance (Ohms)", false, "", ""}},
     computeOhmsVoltage},
    {"Ohm's Law (Power)",
     5,
     2,
     {{"Voltage (V)", false, "", ""}, {"Current (A)", false, "", ""}},
     computeOhmsPower},
    {"Resistors in Series",
     5,
     2,
     {{"R1 (Ohms)", false, "", ""}, {"R2 (Ohms)", false, "", ""}},
     computeResistorsSeries},
    {"Resistors in Parallel",
     5,
     2,
     {{"R1 (Ohms)", false, "", ""}, {"R2 (Ohms)", false, "", ""}},
     computeResistorsParallel},
    {"Voltage Divider",
     5,
     3,
     {{"Vin (V)", false, "", ""},
      {"R1 (Ohms)", false, "", ""},
      {"R2 (Ohms)", false, "", ""}},
     computeVoltageDivider},
};
const int kFormulaCount = sizeof(kFormulas) / sizeof(kFormulas[0]);

struct KeyDef {
  char action;
  const char* label;
  int col;
  int row;
};
const KeyDef kKeypadKeys[FormulasApp::kKeypadKeyCount] = {
    {'7', "7", 0, 0}, {'8', "8", 1, 0}, {'9', "9", 2, 0}, {'D', "DEL", 3, 0},
    {'4', "4", 0, 1}, {'5', "5", 1, 1}, {'6', "6", 2, 1}, {'C', "C", 3, 1},
    {'1', "1", 0, 2}, {'2', "2", 1, 2}, {'3', "3", 2, 2}, {'.', ".", 3, 2},
    {'0', "0", 0, 3},
};
const int kKeypadCols = 4;
const int kKeypadRows = 4;

}  // namespace

void FormulasApp::layoutCategories() {
  const int cols = 3;
  const int rows = 2;
  const int top = kTop + 50;  // leaves room for the "Pick a category" label
  const int areaW = M5.Display.width();
  const int areaH = M5.Display.height() - top;
  const int padding = 20;
  const int tileW = (areaW - padding * (cols + 1)) / cols;
  const int tileH = (areaH - padding * (rows + 1)) / rows;

  for (int i = 0; i < kCategoryCount; ++i) {
    int col = i % cols;
    int row = i / cols;
    int x = padding + col * (tileW + padding);
    int y = top + padding + row * (tileH + padding);
    _categoryButtons[i].initButtonUL(&M5.Display, x, y, tileW, tileH,
                                      Theme::kTextSecondary, Theme::kPrimary,
                                      Theme::kTextPrimary, "", 3.0f, 3.0f);
  }
}

void FormulasApp::layoutList() {
  const int screenW = M5.Display.width();
  const int padding = 8;
  const int btnY = kTop + 10;
  const int btnH = 44;

  _listBackBtn.initButtonUL(&M5.Display, padding, btnY, 150, btnH,
                             Theme::kOutline, Theme::kDanger,
                             Theme::kTextPrimary, "", 2.0f, 2.0f);

  const int listTop = btnY + btnH + 10;
  const int rowH = 56;
  for (int i = 0; i < _listCount; ++i) {
    int y = listTop + i * (rowH + padding);
    _listButtons[i].initButtonUL(&M5.Display, padding, y,
                                  screenW - padding * 2, rowH, Theme::kOutline,
                                  Theme::kPrimary, Theme::kTextPrimary, "",
                                  2.2f, 2.2f);
  }
}

void FormulasApp::layoutKeypad(int x, int y, int w, int h) {
  const int padding = 8;
  const int colW = (w - padding * (kKeypadCols + 1)) / kKeypadCols;
  const int rowH = (h - padding * kKeypadRows) / kKeypadRows;

  for (int i = 0; i < kKeypadKeyCount; ++i) {
    int kx = x + padding + kKeypadKeys[i].col * (colW + padding);
    int ky = y + kKeypadKeys[i].row * (rowH + padding);
    uint32_t fill = (kKeypadKeys[i].action == 'C' ||
                      kKeypadKeys[i].action == 'D')
                         ? Theme::kDanger
                         : Theme::kOutline;
    _keys[i].initButtonUL(&M5.Display, kx, ky, colW, rowH,
                           Theme::kTextSecondary, fill, Theme::kTextPrimary,
                           kKeypadKeys[i].label, 3.0f, 3.0f);
  }
}

void FormulasApp::layoutCalc() {
  const FormulaDef& f = kFormulas[_formulaIndex];
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int padding = 10;

  _calcBackBtn.initButtonUL(&M5.Display, padding, kTop + 10, 150, 44,
                             Theme::kOutline, Theme::kDanger,
                             Theme::kTextPrimary, "", 2.0f, 2.0f);

  _fieldsLeftX = padding;
  _fieldsWidth = (screenW - padding * 3) / 2;
  const int rightX = padding * 2 + _fieldsWidth;
  const int rightW = screenW - rightX - padding;

  const int fieldsTop = kTop + 10 + 44 + 20;
  const int rowH = kFieldLabelH + kFieldGap + kFieldBoxH + kFieldRowGap;

  for (int i = 0; i < f.fieldCount; ++i) {
    int y = fieldsTop + i * rowH;
    _fieldBoxY[i] = y + kFieldLabelH + kFieldGap;
    _fieldButtons[i].initButtonUL(&M5.Display, _fieldsLeftX, _fieldBoxY[i],
                                   _fieldsWidth, kFieldBoxH, Theme::kOutline,
                                   Theme::kSurface, Theme::kTextPrimary, "",
                                   2.2f, 2.2f);
  }

  const int calcY = fieldsTop + f.fieldCount * rowH + 6;
  _calculateBtn.initButtonUL(&M5.Display, _fieldsLeftX, calcY, _fieldsWidth,
                              56, Theme::kOutline, Theme::kSuccess,
                              Theme::kTextPrimary, "", 2.4f, 2.4f);

  _resultX = _fieldsLeftX;
  _resultW = _fieldsWidth;
  _resultTop = calcY + 56 + 16;
  _resultH = screenH - _resultTop - padding;

  layoutKeypad(rightX, fieldsTop, rightW, screenH - fieldsTop - padding);
}

void FormulasApp::enterCategory(int category) {
  _category = category;
  _listCount = 0;
  for (int i = 0; i < kFormulaCount && _listCount < kMaxFormulasPerCategory;
       ++i) {
    if (kFormulas[i].category == category) {
      _listIndices[_listCount++] = i;
    }
  }
  layoutList();
  _state = State::List;
  M5.Display.fillScreen(Theme::kBackground);
}

void FormulasApp::enterFormula(int formulaIndex) {
  _formulaIndex = formulaIndex;
  const FormulaDef& f = kFormulas[formulaIndex];

  _focusedField = -1;
  for (int i = 0; i < f.fieldCount; ++i) {
    strcpy(_fieldText[i], "0");
    _fieldToggle[i] = false;
    if (!f.fields[i].isToggle && _focusedField < 0) _focusedField = i;
  }
  _resultText[0] = 0;

  layoutCalc();
  _state = State::Calc;
  M5.Display.fillScreen(Theme::kBackground);
}

void FormulasApp::appendChar(char c) {
  if (_focusedField < 0) return;
  char* buf = _fieldText[_focusedField];
  int len = (int)strlen(buf);
  if (strcmp(buf, "0") == 0 && c != '.') {
    buf[0] = c;
    buf[1] = 0;
    return;
  }
  if (c == '.' && strchr(buf, '.')) return;
  if (len < (int)sizeof(_fieldText[0]) - 1) {
    buf[len] = c;
    buf[len + 1] = 0;
  }
}

void FormulasApp::backspace() {
  if (_focusedField < 0) return;
  char* buf = _fieldText[_focusedField];
  int len = (int)strlen(buf);
  if (len <= 1) {
    strcpy(buf, "0");
    return;
  }
  buf[len - 1] = 0;
}

void FormulasApp::calculate() {
  const FormulaDef& f = kFormulas[_formulaIndex];
  double values[kMaxFields] = {0, 0, 0, 0};
  for (int i = 0; i < f.fieldCount; ++i) {
    values[i] = f.fields[i].isToggle ? (_fieldToggle[i] ? 1.0 : 0.0)
                                      : atof(_fieldText[i]);
  }
  f.compute(values, _resultText, sizeof(_resultText));
}

void FormulasApp::onEnter() {
  _state = State::Categories;
  layoutCategories();
  M5.Display.fillScreen(Theme::kBackground);
}

void FormulasApp::updateCategories() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < kCategoryCount; ++i) {
    bool inside = pressed && _categoryButtons[i].contains(tx, ty);
    _categoryButtons[i].press(inside);
    if (_categoryButtons[i].justReleased()) {
      enterCategory(i);
      return;
    }
  }
}

void FormulasApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < _listCount; ++i) {
    bool inside = pressed && _listButtons[i].contains(tx, ty);
    _listButtons[i].press(inside);
    if (_listButtons[i].justReleased()) {
      enterFormula(_listIndices[i]);
      return;
    }
  }

  bool insideBack = pressed && _listBackBtn.contains(tx, ty);
  _listBackBtn.press(insideBack);
  if (_listBackBtn.justReleased()) {
    layoutCategories();
    _state = State::Categories;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void FormulasApp::updateCalc() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideBack = pressed && _calcBackBtn.contains(tx, ty);
  _calcBackBtn.press(insideBack);
  if (_calcBackBtn.justReleased()) {
    layoutList();
    _state = State::List;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  const FormulaDef& f = kFormulas[_formulaIndex];
  for (int i = 0; i < f.fieldCount; ++i) {
    bool inside = pressed && _fieldButtons[i].contains(tx, ty);
    _fieldButtons[i].press(inside);
    if (_fieldButtons[i].justReleased()) {
      if (f.fields[i].isToggle) {
        _fieldToggle[i] = !_fieldToggle[i];
      } else {
        _focusedField = i;
      }
      return;
    }
  }

  bool insideCalc = pressed && _calculateBtn.contains(tx, ty);
  _calculateBtn.press(insideCalc);
  if (_calculateBtn.justReleased()) {
    calculate();
    return;
  }

  if (_focusedField < 0) return;
  for (int i = 0; i < kKeypadKeyCount; ++i) {
    bool inside = pressed && _keys[i].contains(tx, ty);
    _keys[i].press(inside);
    if (_keys[i].justReleased()) {
      char a = kKeypadKeys[i].action;
      if (a == 'C') {
        strcpy(_fieldText[_focusedField], "0");
      } else if (a == 'D') {
        backspace();
      } else {
        appendChar(a);
      }
      return;
    }
  }
}

void FormulasApp::update() {
  switch (_state) {
    case State::Categories:
      updateCategories();
      break;
    case State::List:
      updateList();
      break;
    case State::Calc:
      updateCalc();
      break;
  }
}

void FormulasApp::drawCategories() {
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Pick a category", 10, kTop + 12);

  for (int i = 0; i < kCategoryCount; ++i) {
    _categoryButtons[i].drawButton(false, kCategoryNames[i]);
  }
}

void FormulasApp::drawList() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.fillRect(160, kTop + 10, M5.Display.width() - 320, 44,
                       Theme::kBackground);
  M5.Display.drawString(kCategoryNames[_category], M5.Display.width() / 2,
                         kTop + 32);

  for (int i = 0; i < _listCount; ++i) {
    _listButtons[i].drawButton(false, kFormulas[_listIndices[i]].name);
  }
  _listBackBtn.drawButton(false, "Back");
}

void FormulasApp::drawCalc() {
  const FormulaDef& f = kFormulas[_formulaIndex];
  const int screenW = M5.Display.width();

  _calcBackBtn.drawButton(false, "Back");

  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.fillRect(180, kTop + 10, screenW - 190, 44, Theme::kBackground);
  M5.Display.drawString(f.name, 180, kTop + 24);

  for (int i = 0; i < f.fieldCount; ++i) {
    int labelY = _fieldBoxY[i] - kFieldLabelH - kFieldGap;
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
    M5.Display.setTextSize(2);
    M5.Display.fillRect(_fieldsLeftX, labelY, _fieldsWidth, kFieldLabelH,
                         Theme::kBackground);
    M5.Display.drawString(f.fields[i].label, _fieldsLeftX, labelY);

    if (f.fields[i].isToggle) {
      _fieldButtons[i].drawButton(
          false, _fieldToggle[i] ? f.fields[i].toggleOn : f.fields[i].toggleOff);
    } else {
      _fieldButtons[i].drawButton(false, _fieldText[i]);
      if (_focusedField == i) {
        M5.Display.drawRoundRect(_fieldsLeftX - 3, _fieldBoxY[i] - 3,
                                  _fieldsWidth + 6, kFieldBoxH + 6, 10,
                                  Theme::kPrimary);
        M5.Display.drawRoundRect(_fieldsLeftX - 2, _fieldBoxY[i] - 2,
                                  _fieldsWidth + 4, kFieldBoxH + 4, 10,
                                  Theme::kPrimary);
      }
    }
  }

  _calculateBtn.drawButton(false, "Calculate");

  drawWrappedText(&M5.Display, _resultX, _resultTop, _resultW, _resultH,
                   _resultText, 2);

  for (int i = 0; i < kKeypadKeyCount; ++i) _keys[i].drawButton();
}

void FormulasApp::draw() {
  switch (_state) {
    case State::Categories:
      drawCategories();
      break;
    case State::List:
      drawList();
      break;
    case State::Calc:
      drawCalc();
      break;
  }
}
