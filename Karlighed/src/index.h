const char indexPage[] PROGMEM = R"INDEXRAW(
<!DOCTYPE html>
<head><link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/mini.css/3.0.1/mini-default.min.css"></head>
<body>
<form id=form1 action=/modes method=POST><div id=modes></div><input type=submit value=Save></form>
<script>
document.addEventListener('DOMContentLoaded', (event) => {writePos(1);writePos(2);writePos(3);writePos(4);getOld();});
function getOld() {
fetch("/modes")
.then(data=>{return data.json()})
.then(res=>{
for (const key of Object.keys(res)) {
var els = document.getElementsByName(key);
if (!els)return;
if (els.length == 1) {els[0].value = res[key];}
else {
// Select radio with right value
for (i = 0; i<els.length; i++) {
if (els[i].value == res[key]) {els[i].checked=true;break;}
}
}
}})
}
function mod(pos, mi, mn) {return `<input type=radio required name=mod${pos} value=${mi}><label>${mn}</label>`}
function writePos(pos) {
let col = "<br>Color <input name=col"+pos+" type='color' value='#FF4444'>"
let spe = "&nbsp;&nbsp;&nbsp;Speed(sec) <input name=spe"+pos+" type='number' min=0 max=255 value=10>"
let whi = "<br>White %<input name=whi"+pos+" type='number' min=0 max=100 value=50>"
var m = document.getElementById("modes");
let d = document.createElement("fieldset")
d.innerHTML = "<legend>Pos " + pos +"</legend>";
d.innerHTML += mod(pos, 1, "Fix")+mod(pos, 2, "Flow")+mod(pos, 3, "Rainbow")+mod(pos, 4, "Fire");
d.innerHTML += col+spe+whi;
m.appendChild(d)
}
</script></body>
)INDEXRAW";
// const char indexPage[] PROGMEM = R"====(
// <h1>HEJ</h1>
// )====";
