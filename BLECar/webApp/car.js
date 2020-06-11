// Using web-bluetooth. Works on some browsers an OS.
// https://webbluetoothcg.github.io/web-bluetooth/

const serviceUuid = "19b10020-e8f2-537e-4f6c-d104768a1214";

let bluetoothDevice
let ctrlCharacteristic2;
let notifyCharacteristic2;

let buttonValue = 0;
const h = 300;
const w = 300;
const offset = 15;
const offsetR = 40;

sendInProgress = false;
let serviceInput

let connectDisconnectButton
const connectStr = "Connect and drive"
const disconnectStr = "Disconnect"

// TODO: Show on screen if connected or not
// TODO: Listen to disconnect
// TODO: Is notify channel needed?
// TODO: Switch between touch and gyro.

function setup() {
  createCanvas(h, w);
  background("#366");

  serviceInput = createInput(serviceUuid);
  serviceInput.position(15,315);
  serviceInput.size(235, 20)

  connectDisconnectButton = createButton()
  connectDisconnectButton.mousePressed(connectBle);
  connectDisconnectButton.position(15,345);
  connectDisconnectButton.html(connectStr)
  connectDisconnectButton.style('background-color', "#BBBBBB");

  let x = w/2
  let y = h/14
  fill(0);//Black 
  textAlign(CENTER);
  textSize(14);
  text("Forward", x, y)

  // Time to write some cool graphics
  fill(204, 101, 192, 127);
  // Point FW
  x = w/2
  y = h/10
  let arrWith = 40;
  let arrHeight = 20;
  triangle(x, y, x + arrWith, y+arrHeight, x - arrWith, y+arrHeight)
  textAlign(CENTER);

  // Point back
  x = w/2
  y = h - h/10
  arrWith = 40;
  arrHeight = 20;
  triangle(x, y, x + arrWith, y-arrHeight, x - arrWith, y-arrHeight)

  // Point left
  x = w/10
  y = h/2
  arrWith = 40;
  arrHeight = 20;
  triangle(x, y, x + arrHeight, y-arrWith, x + arrHeight, y+arrWith)

  // Point right
  x = w - w/10
  y = h/2
  arrWith = 40;
  arrHeight = 20;
  triangle(x, y, x - arrHeight, y-arrWith, x - arrHeight, y+arrWith)

  // Center
  ellipse(h/2, w/2, 20, 20)

}

function draw() {
  noStroke();
  if (mouseIsPressed) {
    writeSteerSpeed(mouseX, mouseY)
  }
}

function connectBle() {
  let vv = connectDisconnectButton.html();
  if (vv == disconnectStr) {
    disconnectBle();
    return;
  }

  let serviceName = serviceInput.value()

  navigator.bluetooth.requestDevice({
  filters: [{
    services: [serviceName]
  }]
})
.then(
  device => {
    bluetoothDevice = device;
    return device.gatt.connect();
  }
)
.then(
  // TODO: If i filter on "name: "BLECare", how do I get serviceName here?
  server => server.getPrimaryService(serviceName)
)
.then(service => service.getCharacteristics())
.then(characteristics => {
  characteristics.forEach(characteristic => {
    console.log(characteristic.properties.write);
    if (characteristic.properties.write) {
      // Save char so we can write to it later
      ctrlCharacteristic2 = characteristic
    }
  })
  connectDisconnectButton.html(disconnectStr);
  connectDisconnectButton.style('background-color', "#8888BB");
})
.catch(
  error => { console.log(error); });
}

function disconnectBle() {
  if (!bluetoothDevice) {
    return;
  }
  log('Disconnecting from Bluetooth Device...');
  if (bluetoothDevice.gatt.connected) {
    bluetoothDevice.gatt.disconnect();
    connectDisconnectButton.html(connectStr)
    connectDisconnectButton.style('background-color', "#BBBBBB");
  } else {
    log('> Bluetooth Device is already disconnected');
  }
}

function normalizeSteer(v,range) {
  if (v<0) return 0
  if (v>w) return range
  let r = 0
  r += (range*v)/h
  return int(r);
}

// Translate canvas pos to 0-255 
// Top of canvas is max speed (255)
function normalizeSpeed(v,range) {
  if (v<0) return range
  if (v>h) return 0
  let r = range
  r -= (range*v)/h
  return int(r)
}

function writeSteerSpeed(st, sp) {

  // Do not send if previous send is in progress
  if (sendInProgress) {
    console.log("sendInProgress");
    return
  }

  spN = normalizeSpeed(sp, 127)
  stN = normalizeSteer(st,127)
  console.log(`Sending steer:${stN} speed:${spN}`);

  if (ctrlCharacteristic2) {
    let arr = new Uint8Array([spN,stN])
    sendInProgress = true;
    ctrlCharacteristic2.writeValue(arr)
    .then(()=>{
      console.log("Send2 done!")
      sendInProgress = false;
    }, ()=> {
      console.log("Send2 ERR")
      sendInProgress = false;
    })
  } else {
    console.log("Not connected")

  }
}
