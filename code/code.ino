#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Keypad.h>
#include <Servo.h>

// LCD 16x2 on I2C (Address 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 4x4 Keypad Setup (Standard Matrix Fixed)
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'3', '2', '1', 'A'},
  {'6', '5', '4', 'B'},
  {'9', '8', '7', 'C'},
  {'#', '0', '*', 'D'}
};

byte rowPins[ROWS] = {D0, D3, D4, D5};
byte colPins[COLS] = {D8, D7, D6, 3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo Setup
Servo doorServo;
const int servoPin = 1;

// Single Password & State for both Keypad and Web UI
String systemPassword = "2222";
String enteredPassword = "";

bool doorOpen = false;

// Wi-Fi Access Point Details
const char* wifiName = "SmartHome";
const char* wifiPassword = "12345678";

ESP8266WebServer server(80);
unsigned long startupTime = 0;

// LCD Display Helper: Shows current status and the entered PIN
void showPasswordScreen() {
  lcd.clear();

  if (!doorOpen) {
    lcd.setCursor(0, 0);
    lcd.print("Enter Password:");

    lcd.setCursor(0, 1);
    lcd.print("PIN: ");
    lcd.print(enteredPassword);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("DOOR IS OPEN");

    lcd.setCursor(0, 1);
    lcd.print("PIN: ");
    lcd.print(enteredPassword);
  }
}

// Open Door (Servo -> 180 degrees)
void openDoor() {
  doorServo.write(180);
  doorOpen = true;
  enteredPassword = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ACCESS GRANTED");
  lcd.setCursor(0, 1);
  lcd.print("DOOR OPEN");

  delay(1500);
  showPasswordScreen();
}

// Close Door (Servo -> 0 degrees)
void closeDoor() {
  doorServo.write(0);
  doorOpen = false;
  enteredPassword = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DOOR LOCKED");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM SECURED");

  delay(1500);
  showPasswordScreen();
}

// HTML Mobile Web UI - Single Page App with Keypad
String makeWebPage() {
  String page = "";
  page += "<!DOCTYPE html><html lang='en'><head>";
  page += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>";
  page += "<title>Smart Home Security</title>";
  page += "<style>";
  page += ":root{";
  page += "--bg:#080b11;--panel:#141b25;--panel-2:#0e141d;--border:#242e3c;";
  page += "--text:#e8eef6;--muted:#66748a;--key:#182130;--key-hi:#212c3d;";
  page += "--locked:#ef4444;--locked-glow:rgba(239,68,68,.5);";
  page += "--open:#22d46f;--open-glow:rgba(34,212,111,.5);";
  page += "--accent:#38bdf8;}";
  page += "*{box-sizing:border-box;}";
  page += "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px;";
  page += "font-family:-apple-system,'Segoe UI',Roboto,sans-serif;color:var(--text);";
  page += "background:radial-gradient(circle at 50% 0%,#101823 0%,var(--bg) 65%);}";
  page += ".panel{position:relative;width:100%;max-width:340px;background:linear-gradient(180deg,var(--panel),var(--panel-2));";
  page += "border:1px solid var(--border);border-radius:26px;padding:30px 24px 26px;";
  page += "box-shadow:0 30px 70px rgba(0,0,0,.55), inset 0 1px 0 rgba(255,255,255,.05);}";
  page += ".rivet{position:absolute;width:6px;height:6px;border-radius:50%;background:radial-gradient(circle at 35% 35%,#3a4557,#0c1119);}";
  page += ".rivet.tl{top:14px;left:14px;}.rivet.tr{top:14px;right:14px;}.rivet.bl{bottom:14px;left:14px;}.rivet.br{bottom:14px;right:14px;}";
  page += ".eyebrow{text-align:center;font-family:'SF Mono',Consolas,'Courier New',monospace;";
  page += "font-size:10px;letter-spacing:4px;color:var(--muted);margin:0 0 6px;}";
  page += "h1{text-align:center;font-size:19px;letter-spacing:.5px;margin:0 0 18px;color:var(--text);font-weight:700;}";
  page += ".lock{width:26px;height:24px;margin:0 auto 16px;position:relative;}";
  page += ".lock .shackle{position:absolute;top:0;left:50%;width:15px;height:14px;transform:translateX(-50%);";
  page += "border:3px solid var(--locked);border-bottom:none;border-radius:14px 14px 0 0;transition:.35s ease;}";
  page += ".lock .body{position:absolute;bottom:0;left:0;width:26px;height:13px;border-radius:4px;background:var(--locked);";
  page += "box-shadow:0 0 18px var(--locked-glow);transition:.35s ease;}";
  page += ".lock.open .shackle{border-color:var(--open);transform:translateX(-50%) translateY(-5px) rotate(-22deg);}";
  page += ".lock.open .body{background:var(--open);box-shadow:0 0 18px var(--open-glow);}";
  page += ".status{display:flex;align-items:center;justify-content:center;gap:8px;";
  page += "font-family:'SF Mono',Consolas,'Courier New',monospace;font-size:12px;letter-spacing:2px;";
  page += "color:var(--muted);margin-bottom:22px;}";
  page += ".status .dot{width:8px;height:8px;border-radius:50%;background:var(--locked);box-shadow:0 0 10px var(--locked-glow);";
  page += "animation:pulse 1.8s ease-in-out infinite;}";
  page += ".status.open .dot{background:var(--open);box-shadow:0 0 10px var(--open-glow);}";
  page += "@keyframes pulse{0%,100%{opacity:1;}50%{opacity:.35;}}";
  page += ".pindots{display:flex;justify-content:center;gap:16px;background:var(--panel-2);";
  page += "border:1px solid var(--border);border-radius:14px;padding:18px 10px;margin-bottom:22px;}";
  page += ".pindots span{width:14px;height:14px;border-radius:50%;border:2px solid var(--border);transition:.15s ease;}";
  page += ".pindots span.filled{background:var(--accent);border-color:var(--accent);box-shadow:0 0 10px rgba(56,189,248,.6);}";
  page += ".lockNow{width:100%;height:46px;margin-bottom:18px;border-radius:12px;border:1px solid var(--locked);";
  page += "background:rgba(239,68,68,.12);color:var(--locked);font-family:'SF Mono',Consolas,'Courier New',monospace;";
  page += "font-size:13px;letter-spacing:2px;font-weight:700;cursor:pointer;box-shadow:none;transition:.15s ease;}";
  page += ".lockNow:active{background:rgba(239,68,68,.22);transform:translateY(1px);}";
  page += ".keypad{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;}";
  page += "button{background:linear-gradient(180deg,var(--key-hi),var(--key));border:1px solid var(--border);";
  page += "color:var(--text);font-size:20px;font-weight:600;padding:0;height:56px;border-radius:14px;cursor:pointer;";
  page += "box-shadow:0 4px 0 #0a0f16,0 8px 14px rgba(0,0,0,.35);transition:transform .08s ease,box-shadow .08s ease;}";
  page += "button:active{transform:translateY(3px);box-shadow:0 1px 0 #0a0f16,0 3px 6px rgba(0,0,0,.35);}";
  page += "button.action{color:var(--accent);font-size:16px;letter-spacing:1px;}";
  page += "</style></head><body>";

  page += "<div class='panel'>";
  page += "<div class='rivet tl'></div><div class='rivet tr'></div><div class='rivet bl'></div><div class='rivet br'></div>";
  page += "<p class='eyebrow'>SMART HOME SECURITY</p>";
  page += "<h1>Front Door Access</h1>";

  if (doorOpen) {
    page += "<div class='lock open' id='lockIcon'><div class='shackle'></div><div class='body'></div></div>";
    page += "<div id='status' class='status open'><span class='dot'></span>DOOR UNLOCKED</div>";
  } else {
    page += "<div class='lock' id='lockIcon'><div class='shackle'></div><div class='body'></div></div>";
    page += "<div id='status' class='status'><span class='dot'></span>DOOR LOCKED</div>";
  }

  page += "<div class='pindots' id='pindots'>";
  page += "<span></span><span></span><span></span><span></span>";
  page += "</div>";

  if (doorOpen) {
    page += "<button class='lockNow' id='lockNowBtn' onclick='closeDoorNow()'>LOCK DOOR NOW</button>";
  } else {
    page += "<button class='lockNow' id='lockNowBtn' style='display:none' onclick='closeDoorNow()'>LOCK DOOR NOW</button>";
  }

  page += "<div class='keypad'>";
  page += "<button onclick='press(\"1\")'>1</button>";
  page += "<button onclick='press(\"2\")'>2</button>";
  page += "<button onclick='press(\"3\")'>3</button>";
  page += "<button onclick='press(\"4\")'>4</button>";
  page += "<button onclick='press(\"5\")'>5</button>";
  page += "<button onclick='press(\"6\")'>6</button>";
  page += "<button onclick='press(\"7\")'>7</button>";
  page += "<button onclick='press(\"8\")'>8</button>";
  page += "<button onclick='press(\"9\")'>9</button>";
  page += "<button class='action' onclick='press(\"*\")'>CLR</button>";
  page += "<button onclick='press(\"0\")'>0</button>";
  page += "<button class='action' onclick='press(\"#\")'>OK</button>";
  page += "</div></div>";

  // JavaScript for Single-Page logic
  page += "<script>";
  page += "let pin='';";
  page += "const dots=document.querySelectorAll('#pindots span');";
  page += "const statusDiv=document.getElementById('status');";
  page += "const lockIcon=document.getElementById('lockIcon');";
  page += "const lockNowBtn=document.getElementById('lockNowBtn');";

  page += "function setLockedUI(){";
  page += "  statusDiv.className='status'; statusDiv.innerHTML='<span class=\"dot\"></span>DOOR LOCKED';";
  page += "  lockIcon.className='lock'; lockNowBtn.style.display='none';";
  page += "}";
  page += "function setOpenUI(){";
  page += "  statusDiv.className='status open'; statusDiv.innerHTML='<span class=\"dot\"></span>DOOR UNLOCKED';";
  page += "  lockIcon.className='lock open'; lockNowBtn.style.display='block';";
  page += "}";

  page += "function closeDoorNow(){";
  page += "  lockNowBtn.disabled=true;";
  page += "  fetch('/close',{method:'POST'})";
  page += "  .then(response=>response.text())";
  page += "  .then(data=>{ if(data==='CLOSED'){ setLockedUI(); } lockNowBtn.disabled=false; })";
  page += "  .catch(()=>{ lockNowBtn.disabled=false; });";
  page += "}";

  page += "function updateDisplay(){";
  page += "  dots.forEach((d,i)=>{ d.className = i < pin.length ? 'filled' : ''; });";
  page += "}";

  page += "function press(key){";
  page += "  if(key==='*'){ pin=''; updateDisplay(); }";
  page += "  else if(key==='#'){ if(pin.length>0) submitPin(); }";
  page += "  else if(pin.length<4){ pin+=key; updateDisplay(); }";
  page += "}";

  page += "function submitPin(){";
  page += "  fetch('/submit',{";
  page += "    method:'POST',";
  page += "    headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  page += "    body:'pin='+pin";
  page += "  })";
  page += "  .then(response=>{";
  page += "    if(response.ok) return response.text();";
  page += "    throw new Error('Invalid');";
  page += "  })";
  page += "  .then(data=>{";
  page += "    if(data==='OPEN'){ setOpenUI(); } else { setLockedUI(); }";
  page += "    pin=''; updateDisplay();";
  page += "  })";
  page += "  .catch(err=>{";
  page += "    statusDiv.className='status'; statusDiv.innerHTML='<span class=\"dot\"></span>WRONG PIN';";
  page += "    setTimeout(()=>{ pin=''; updateDisplay(); lockIcon.className.includes('open') ? setOpenUI() : setLockedUI(); }, 1500);";
  page += "  });";
  page += "}";
  page += "</script>";
  page += "</body></html>";

  return page;
}

// Handle Web Dashboard Home Page
void handleHome() {
  server.send(200, "text/html", makeWebPage());
}

// Handle Mobile Keypad Submission
void handleSubmit() {
  if (!server.hasArg("pin")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  String enteredPin = server.arg("pin");

  if (enteredPin == systemPassword) {
    if (!doorOpen) {
      openDoor();
    } else {
      closeDoor();
    }
    // Return new state to the JavaScript client
    server.send(200, "text/plain", doorOpen ? "OPEN" : "CLOSED");
  } else {
    // Send 401 Unauthorized for wrong PIN
    server.send(401, "text/plain", "DENIED");
  }
}

// Handle "Lock Door Now" button from the web UI
void handleClose() {
  if (doorOpen) {
    closeDoor();
  }
  server.send(200, "text/plain", "CLOSED");
}

void setup() {
  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();

  doorServo.attach(servoPin);
  doorServo.write(0); // Start locked
  doorOpen = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SMART SECURITY");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  delay(1500);

  // Setup Wi-Fi SoftAP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(wifiName, wifiPassword);

  // Web Server Routes
  server.on("/", HTTP_GET, handleHome);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/close", HTTP_POST, handleClose);

  server.begin();

  enteredPassword = "";
  showPasswordScreen();

  startupTime = millis();
}

void loop() {
  server.handleClient();

  if (millis() - startupTime < 2500) {
    return;
  }

  char key = keypad.getKey();

  if (!key) {
    return;
  }

  if (key == '*') {
    // Clear entered PIN
    enteredPassword = "";
    showPasswordScreen();
  }
  else if (key == '#') {
    // Check PIN
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Checking...");

    delay(500);

    // Using the unified systemPassword
    if (enteredPassword == systemPassword) {
      if (!doorOpen) {
        // Door is locked -> Unlock it
        openDoor();
      } else {
        // Door is open -> Lock it
        closeDoor();
      }
    } else {
      // Wrong PIN
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCESS DENIED");
      lcd.setCursor(0, 1);
      lcd.print("WRONG PASSWORD");

      delay(2000);

      enteredPassword = "";
      showPasswordScreen();
    }
  }
  else if (key == 'A') {
    // Manual override: close/lock the door directly, no PIN required
    if (doorOpen) {
      closeDoor();
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DOOR ALREADY");
      lcd.setCursor(0, 1);
      lcd.print("LOCKED");

      delay(1200);

      showPasswordScreen();
    }
  }
  else if (key >= '0' && key <= '9') {
    // Collect numbers up to 4 digits and display on LCD
    if (enteredPassword.length() < 4) {
      enteredPassword += key;
      showPasswordScreen();
    }
  }
}