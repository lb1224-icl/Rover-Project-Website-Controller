let locked = [0,0,0,0];
let lockedIndex = ["RF", "IR", "NAME", "MAGNET"];

let duckCount = 0;

let currentIR = 0;
let currentRF = 0;
let currentMagnetic = "South";

function changeClass(elm) {
    if (locked[lockedIndex.indexOf(elm.id)] == 0){
        //enable
        elm.className = elm.className.replace( /(?:^|\s)card2(?!\S)/g , '' );
        elm.classList.add("card3");
        locked[lockedIndex.indexOf(elm.id)] = 1;
    }
    else{
        //disable
        elm.className = elm.className.replace( /(?:^|\s)card3(?!\S)/g , '' );
        elm.classList.add("card2");
        locked[lockedIndex.indexOf(elm.id)] = 0;
    }
}

function getSpecies(inputIr, inputRf, inputMagnetic) {
  // Define tolerance in Hz
  const tolerance = 20; 

  // Species data with frequencies and magnetic direction
  const speciesData = [
    { name: "Wibbo", ir: 457, rf: null, magnetic: "South"},
    { name: "Gribbit", ir: null, rf: 100, magnetic: "South"},
    { name: "Snorkle", ir: 293, rf: null, magnetic: "North"},
    { name: "Zapple", ir: null, rf: 150, magnetic: "North"}
  ];

  for (const species of speciesData) {
    const irMatch = species.ir === null || (inputIr !== null && Math.abs(species.ir - inputIr) <= tolerance);
    const rfMatch = species.rf === null || (inputRf !== null && Math.abs(species.rf - inputRf) <= tolerance);
    const magMatch = species.magnetic.toLowerCase() === inputMagnetic.toLowerCase();

    if (irMatch && rfMatch && magMatch) {
      return species.name;
    }
  }

  return 'Unknown species';
}

function checkLockData(){
  if(!locked.includes(0)){
    lockData();
  }
}

function addToDuckTable(_name, species, ir, rf, magnet){
  var row = document.getElementById(duckCount);

  var cell1 = row.children[0];
  var cell2 = row.children[1];
  var cell3 = row.children[2];
  var cell4 = row.children[3];
  var cell5 = row.children[4];

  cell1.innerHTML = _name;
  cell2.innerHTML = species;
  cell3.innerHTML = ir;
  cell4.innerHTML = rf;
  cell5.innerHTML = magnet;
}

function lockData(){
  //add data to a list
  ir = document.getElementById("ir").textContent;
  rf = document.getElementById("rf").textContent;
  _name = document.getElementById("name").textContent;
  magnet = document.getElementById("magnet").textContent;
  species = getSpecies(parseInt(ir), parseInt(rf), magnet);
  duckCount++;

  addToDuckTable(_name, species, ir, rf, magnet);

  //reset all looks
  locked = [0, 0, 0, 0];  
  const elements = document.querySelectorAll('.card3');
  Array.from(elements).forEach((element, index) => {
  element.className = element.className.replace( /(?:^|\s)card3(?!\S)/g , '' );
  element.classList.add("card2");
  });
}

function removeLast(){
  if(duckCount>0){
    addToDuckTable("N/A", "N/A", "N/A", "N/A", "N/A");
    duckCount--;
  }
}

function updateRadio(text){
  if(locked[0] == 0){
    currentRF = text;
    document.getElementById("rf").textContent = text + "Hz";
  }
}

function updateIR(text){
  if(locked[1] == 0){
    currentIR = text;
    document.getElementById("ir").textContent = text + "Hz";
  }
}

function updateName(text){
  if(locked[2] == 0){
    document.getElementById("name").textContent = text;
  }
}

function updateMagnet(text){
  if(locked[3] == 0){
    currentMagnetic = text;
    document.getElementById("magnet").textContent = text;
  }
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    updateRadio(myObj.RF);
    updateIR(myObj.IR);
    updateMagnet(myObj.MAGNETIC);
    updateName(myObj.NAME);
  }, false);

  //might need some tweaks as e.data might be a json file not a string
  source.addEventListener('buttonPressed', function(e) {
    const dataInt = parseInt(e.data, 10); 
    console.log("Button Pressed: " + dataInt);
    if(dataInt < 4){
      if(locked[dataInt] == 0)
      {
        locked[dataInt] = 1;
        let elm = document.getElementById(lockedIndex[dataInt]);
        elm.className = elm.className.replace(/(?:^|\s)card2(?!\S)/g, '');
        elm.classList.add("card3");
      }
      else{
        locked[dataInt] = 0;
        let elm = document.getElementById(lockedIndex[dataInt]);
        elm.className = elm.className.replace(/(?:^|\s)card3(?!\S)/g, '');
        elm.classList.add("card2");
      }
    }
    else if(dataInt == 4){
      // left bumper
      checkLockData();
    }
    else if(dataInt == 5){
      // right bumper
      removeLast();
    }
}, false);
}

let currentCommand = {
    forward: 0,
    backward: 0,
    left: 0,
    right: 0,
    specialLeft: 0,
    specialRight: 0
};

document.addEventListener('keydown', handleKeyEvent);
document.addEventListener('keyup', handleKeyEvent);

function handleKeyEvent(e) {
    const isKeyDown = e.type === 'keydown';
    const key = e.key.toLowerCase();

    switch(key) {
        case 'w': currentCommand.forward = isKeyDown ? 1 : 0; break;
        case 's': currentCommand.backward = isKeyDown ? 1 : 0; break;
        case 'a': currentCommand.left = isKeyDown ? 1 : 0; break;
        case 'd': currentCommand.right = isKeyDown ? 1 : 0; break;
        case 'q': currentCommand.specialLeft = isKeyDown ? 1 : 0; break;
        case 'e': currentCommand.specialRight = isKeyDown ? 1 : 0; break;
        default: return;
    }

    console.log(currentCommand.forward);

    e.preventDefault();
    fetch('/controllerdata', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            forward: currentCommand.forward,
            backward: currentCommand.backward,
            left: currentCommand.left,
            right: currentCommand.right,
            specialLeft: currentCommand.specialLeft,
            specialRight: currentCommand.specialRight
        })
    });
}

function sendMotorCommand() {
    fetch('/controllerdata', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            forward: currentCommand.forward,
            backward: currentCommand.backward,
            left: currentCommand.left,
            right: currentCommand.right,
            specialLeft: currentCommand.specialLeft,
            specialRight: currentCommand.specialRight
        })
    });
}

