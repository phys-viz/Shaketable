#include <WiFi.h>
#include <WebServer.h>
#include "Presets.h"

// Classic ESP32 DevKit / WROOM pins. Confirm your board before wiring.
constexpr int ENA = 25, IN1 = 26, IN2 = 27;
constexpr uint32_t TIMEOUT_MS = 2000;
const char *AP_NAME = "MotorController";
const char *AP_PASSWORD = "motorcontrol"; // Change this; minimum 8 characters.
WebServer server(80);
bool pwmReady = false;
int duty = 0;
const Segment *active = nullptr;
unsigned segmentCount = 0;
uint32_t presetStarted = 0, lastHeartbeat = 0, presetDuration = 0;
String mode = "Stopped";

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Shake table</title>
<style>
body{font:18px system-ui;background:#101827;color:#edf2fa;margin:0;padding:20px}
main{max-width:540px;margin:20px auto;background:#1c293c;padding:24px;border-radius:20px}
input[type=range]{width:100%;margin:20px 0}input[type=number]{font:inherit;width:100px;padding:8px}
button{font:inherit;padding:14px;border:0;border-radius:10px;cursor:pointer;margin:6px 0;width:100%}
.row{display:flex;gap:10px}.stop{background:#e54a50;color:white}p,small{color:#bac9df}
output{font-weight:bold}progress{width:100%}
</style><main><h1>Shake table</h1>
<p id="status" role="status">Connecting...</p>
<p>Actual PWM: <output id="actual">0 / 255</output><br>Ideal average voltage: <output id="voltage">0 V</output></p>
<label>Manual PWM: <output id="target">0</output><input id="pwm" type="range" min="0" max="255" value="0"></label>
<div class="row"><button id="minus">-5 PWM</button><button id="plus">+5 PWM</button></div>
<button id="full">Full power (255)</button>
<label>Supply voltage (V): <input id="supply" type="number" min="1" max="50" step="0.1" value="12"></label>
<p><label>Ideal average target (V): <input id="volts" type="number" min="0" step="0.1" value="0"></label></p>
<button id="apply">Apply voltage target</button>
<small>Voltage = supply x PWM / 255, before L298N losses. This is an estimate, not measured or regulated voltage. Supply entry only changes the calculation.</small>
<h2>Presets</h2><button id="motown">Great Motown Earthquake of 2024 (12 s)</button>
<button id="japan">Japanese Earthquake of 2011 (45 s)</button>
<small>Original demonstration sequences; not calibrated reproductions of recorded earthquakes.</small>
<progress id="progress" value="0" max="1"></progress>
<button class="stop" id="stop">STOP / Reset PWM</button>
<p>Keep this page visible. Loss of contact stops the motor within about 2 seconds. Use one controlling device.</p></main>
<script>
const $=id=>document.getElementById(id);
let connected=false,queue=Promise.resolve();
function supply(){return Math.max(1,Math.min(50,Number($('supply').value)||12));}
function select(value){$('pwm').value=Math.max(0,Math.min(255,Math.round(value)));$('target').textContent=$('pwm').value;}
function render(s){
 $('actual').textContent=s.pwm+' / 255';$('voltage').textContent=(supply()*s.pwm/255).toFixed(2)+' V';
 $('status').textContent=s.mode;$('progress').max=s.duration||1;$('progress').value=s.elapsed;
}
async function request(path,data){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),1200);
 try{
  const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(data),signal:controller.signal});
  if(!r.ok)throw Error(await r.text());
  const state=await r.json();connected=true;render(state);return state;
 }finally{clearTimeout(timer);}
}
function failure(){connected=false;select(0);$('status').textContent='Connection lost - motor will stop on timeout';}
function command(data){
 if(!connected)return;
 queue=queue.then(()=>request('/command',data)).catch(failure);
}
$('pwm').oninput=()=>select($('pwm').value);
$('pwm').onchange=()=>command({action:'manual',pwm:$('pwm').value});
$('minus').onclick=()=>{select(Number($('pwm').value)-5);command({action:'manual',pwm:$('pwm').value});};
$('plus').onclick=()=>{select(Number($('pwm').value)+5);command({action:'manual',pwm:$('pwm').value});};
$('full').onclick=()=>{select(255);command({action:'manual',pwm:255});};
$('apply').onclick=()=>{select((Number($('volts').value)||0)/supply()*255);command({action:'manual',pwm:$('pwm').value});};
$('motown').onclick=()=>command({action:'motown'});
$('japan').onclick=()=>command({action:'japan'});
function stop(){select(0);queue=queue.then(()=>request('/command',{action:'stop'})).catch(failure);}
$('stop').onclick=stop;
async function heartbeat(){
 if(!document.hidden){try{await request('/heartbeat',{});}catch(e){failure();}}
 setTimeout(heartbeat,400);
}
function leave(){connected=false;select(0);navigator.sendBeacon('/stop','');}
window.addEventListener('pagehide',leave);
document.addEventListener('visibilitychange',()=>{if(document.hidden)leave();});
heartbeat();
</script></html>
)HTML";

void setPWM(int value) {
  duty = constrain(value, 0, 255);
  // One direction, matching the old Python interface. Swap motor leads if needed.
  digitalWrite(IN1, duty ? HIGH : LOW);
  digitalWrite(IN2, LOW);
  if (pwmReady) ledcWrite(ENA, duty);
}
void stopMotor() {
  active = nullptr;
  presetDuration = 0;
  setPWM(0);
  mode = "Stopped";
}
void updateMotor() {
  const uint32_t now = millis();
  if ((active || duty) && now - lastHeartbeat >= TIMEOUT_MS) {
    stopMotor();
    mode = "Stopped - connection timeout";
  }
  if (active) {
    uint32_t elapsed = now - presetStarted;
    if (elapsed >= presetDuration) stopMotor();
    else setPWM(presetPWM(active, segmentCount, elapsed));
  }
}
void sendState() {
  updateMotor();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", String("{\"pwm\":") + duty
    + ",\"mode\":\"" + mode + "\",\"duration\":" + presetDuration
    + ",\"elapsed\":" + (active ? millis() - presetStarted : 0) + "}");
}
void handleCommand() {
  String action = server.arg("action");
  if (action == "stop") { stopMotor(); sendState(); return; }
  if (!pwmReady || millis() - lastHeartbeat >= TIMEOUT_MS) {
    stopMotor(); server.send(409, "text/plain", "Reconnect before starting"); return;
  }
  if (action == "manual") {
    String value = server.arg("pwm");
    bool valid = value.length() > 0 && value.length() <= 3;
    for (unsigned i = 0; i < value.length(); ++i) valid &= value[i] >= '0' && value[i] <= '9';
    if (!valid || value.toInt() > 255) {
      stopMotor(); server.send(400, "text/plain", "PWM must be 0 through 255"); return;
    }
    stopMotor(); setPWM(value.toInt()); mode = duty ? "Manual" : "Stopped";
  } else if (action == "motown" || action == "japan") {
    stopMotor();
    active = action == "motown" ? MOTOWN : JAPAN;
    segmentCount = action == "motown" ? 2 : 8;
    presetDuration = action == "motown" ? 12000 : 45000;
    presetStarted = millis();
    mode = action == "motown" ? "Great Motown Earthquake of 2024" : "Japanese Earthquake of 2011";
  } else {
    stopMotor(); server.send(400, "text/plain", "Unknown action"); return;
  }
  sendState();
}
void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT); digitalWrite(ENA, LOW);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  pwmReady = ledcAttach(ENA, 1000, 8); // Arduino-ESP32 3.x.
  stopMotor();
  if (!pwmReady) { Serial.println("PWM initialization failed"); return; }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_NAME, AP_PASSWORD)) { Serial.println("Wi-Fi initialization failed"); return; }
  Serial.print("Open http://"); Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, [](){server.send_P(200, "text/html", PAGE);});
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/heartbeat", HTTP_POST, [](){
    updateMotor(); // Enforce expired timeout before accepting renewed contact.
    lastHeartbeat = millis(); sendState();
  });
  server.on("/stop", HTTP_POST, [](){stopMotor(); sendState();});
  server.begin();
}
void loop() { updateMotor(); server.handleClient(); updateMotor(); }
