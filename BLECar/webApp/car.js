const serviceUuid = "19b10020-e8f2-537e-4f6c-d104768a1214";
let ctrlCharacteristic;
let notifyCharacteristic;
let myBLE;

let buttonValue = 0;
const h = 300;
const w = 300;
const offset = 15;
const offsetR = 40;

sendInProgress = false;
let inp

function setup() {
  createCanvas(h, w);
  background("#366");
  myBLE = new p5ble();

  inp = createInput(serviceUuid);
  inp.position(15,315);
  inp.size(225, 20)
//    inp.input(myInputEvent);
  // Create a 'Connect and Start Notifications' button
  const connectButton = createButton('Connect and Drive')
  connectButton.mousePressed(connectAndStartNotify);
  connectButton.position(15,345);

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

function connectAndStartNotify() {
  // Connect to a device by passing the service UUID
  let v = inp.value()
  myBLE.connect(v, gotCharacteristics);
}

// A function that will be called once got characteristics
function gotCharacteristics(error, characteristics) {
  if (!characteristics || error) {
    console.log("Char undefined, error: " , error)
    return
  }

  ctrlCharacteristic = null
  notifyCharacteristic = null
  for(let i=0; i<characteristics.length;i++) {
    // Expecting two 
    let ch = characteristics[i]
    if (ch.properties.write) {
      if (ctrlCharacteristic != null){
        alert("Oops, more than one writable characteristic.")
        return;
      }
      ctrlCharacteristic = ch;
    }
    if (ch.properties.notify) {
      if (notifyCharacteristic != null){
        alert("Oops, more than one notify characteristic.")
        return;
      }
      notifyCharacteristic = ch;

      // No use for notifications right now
      // myBLE.read(notifyCharacteristic, gotValue);
    }
    console.log("c " , i)
    console.log(characteristics[i].uuid)
  }
  if (notifyCharacteristic == null) {
    alert("Oops, no notify characteristic found.")
  }
  if (ctrlCharacteristic == null) {
    alert("Oops, no writable characteristic found.")
  }
}

function gotValue(error, value) {
  if (error) {
    console.log('error: ', error);
    return
  }
  console.log('value: ', value);
  // After getting a value, call p5ble.read() again to get the value again
  myBLE.read(notifyCharacteristic, gotValue);
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
  if (!ctrlCharacteristic) {
    console.log("Not connected");
    return
  }

  // The examples use the method myBLE.write(ctrlCharacteristic, someData)
  // but I was not able to send more that one byte.
  // The undocumented function charactereistcs.writeValue(data) worked great.  
  let arr = new Uint8Array([spN,stN])
  if (ctrlCharacteristic) {
    sendInProgress = true;
    ctrlCharacteristic.writeValue(arr).then(()=>{
      console.log("Send done!")
      sendInProgress = false;
    }, ()=> {
      console.log("Send ERR")
      sendInProgress = false;
    })
  }
}
