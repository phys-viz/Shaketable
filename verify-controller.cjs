const fs = require('fs');
const vm = require('vm');
const assert = require('assert/strict');
const sketch = fs.readFileSync('MotorController/MotorController.ino', 'utf8');
const js = sketch.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(js);
const header = fs.readFileSync('MotorController/Presets.h', 'utf8');
function segments(name) {
 return [...header.match(new RegExp(name+'\\[\\] = \\{([\\s\\S]*?)\\};'))[1].matchAll(/\{(\d+),\s*(\d+),\s*(\d+)\}/g)].map(m=>m.slice(1).map(Number));
}
function at(seq,t) {
 for(const [duration,from,to] of seq){if(t<duration)return from+Math.trunc((to-from)*t/duration);t-=duration;}return 0;
}
const motown=segments('MOTOWN'), japan=segments('JAPAN');
assert.equal(at(motown,0),0);assert.equal(at(motown,1000),62);assert.equal(at(motown,2000),125);
assert.equal(at(motown,11999),125);assert.equal(at(motown,12000),0);
assert.equal(japan.reduce((sum,s)=>sum+s[0],0),45000);
for(const [t,pwm] of [[0,0],[5000,255],[10000,255],[15000,200],[20000,200],[30000,255],[35000,255],[40000,200],[45000,0]])assert.equal(at(japan,t),pwm);
for(let t=0;t<=45000;t++){const pwm=at(japan,t);assert(pwm>=0&&pwm<=255);}
console.log('Passed: embedded JavaScript syntax; preset durations, boundaries, ramp interpolation, and PWM bounds. Hardware compilation not performed.');
