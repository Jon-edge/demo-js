load('api_aws.js');
load('api_azure.js');
load('api_config.js');
load('api_dash.js');
load('api_events.js');
load('api_gcp.js');
load('api_gpio.js');
load('api_mqtt.js');
load('api_shadow.js');
load('api_timer.js');
load('api_sys.js');
load('api_watson.js');
load('api_neopixel.js');

let btn = Cfg.get('board.btn1.pin');              // Built-in button GPIO
let led = Cfg.get('board.led1.pin');              // Built-in LED GPIO number
let onhi = Cfg.get('board.led1.active_high');     // LED on when high?
let state = {on: false, btnCount: 0, uptime: 0};  // Device state
let online = false;                               // Connected to the cloud?

let setLED = function(on) {
  let level = onhi ? on : !on;
  GPIO.write(led, level);
  print('LED on ->', on);
};

GPIO.set_mode(led, GPIO.MODE_OUTPUT);
setLED(state.on);

let reportState = function() {
  Shadow.update(0, state);
};

// Update state every second, and report to cloud if online
// Timer.set(1000, Timer.REPEAT, function() {
//   state.uptime = Sys.uptime();
//   state.ram_free = Sys.free_ram();
//   print('online:', online, JSON.stringify(state));
//   if (online) reportState();
// }, null);

// Set up Shadow handler to synchronise device state with the shadow state
Shadow.addHandler(function(event, obj) {
  if (event === 'UPDATE_DELTA') {
    print('GOT DELTA:', JSON.stringify(obj));
    for (let key in obj) {  // Iterate over all keys in delta
      if (key === 'on') {   // We know about the 'on' key. Handle it!
        state.on = obj.on;  // Synchronise the state
        setLED(state.on);   // according to the delta
      } else if (key === 'reboot') {
        state.reboot = obj.reboot;      // Reboot button clicked: that
        Timer.set(750, 0, function() {  // incremented 'reboot' counter
          Sys.reboot(500);                 // Sync and schedule a reboot
        }, null);
      }
    }
    reportState();  // Report our new state, hopefully clearing delta
  }
});

if (btn >= 0) {
  let btnCount = 0;
  let btnPull, btnEdge;
  if (Cfg.get('board.btn1.pull_up') ? GPIO.PULL_UP : GPIO.PULL_DOWN) {
    btnPull = GPIO.PULL_UP;
    btnEdge = GPIO.INT_EDGE_NEG;
  } else {
    btnPull = GPIO.PULL_DOWN;
    btnEdge = GPIO.INT_EDGE_POS;
  }
  GPIO.set_button_handler(btn, btnPull, btnEdge, 20, function() {
    state.btnCount++;
    let message = JSON.stringify(state);
    let sendMQTT = true;
    if (Azure.isConnected()) {
      print('== Sending Azure D2C message:', message);
      Azure.sendD2CMsg('', message);
      sendMQTT = false;
    }
    if (GCP.isConnected()) {
      print('== Sending GCP event:', message);
      GCP.sendEvent(message);
      sendMQTT = false;
    }
    if (Watson.isConnected()) {
      print('== Sending Watson event:', message);
      Watson.sendEventJSON('ev', {d: state});
      sendMQTT = false;
    }
    if (Dash.isConnected()) {
      print('== Click!');
      // TODO: Maybe do something else?
      sendMQTT = false;
    }
    // AWS is handled as plain MQTT since it allows arbitrary topics.
    if (AWS.isConnected() || (MQTT.isConnected() && sendMQTT)) {
      let topic = 'devices/' + Cfg.get('device.id') + '/events';
      print('== Publishing to ' + topic + ':', message);
      MQTT.pub(topic, message, 0 /* QoS */);
    } else if (sendMQTT) {
      print('== Not connected!');
    }
  }, null);
}

Event.on(Event.CLOUD_CONNECTED, function() {
  online = true;
  Shadow.update(0, {ram_total: Sys.total_ram()});
}, null);

Event.on(Event.CLOUD_DISCONNECTED, function() {
  online = false;
}, null);

print('==================================');
print('==================================');
print('==================================');
print('==================================');

// Initialize the neopixel strip
let strip = NeoPixel.create(5, 512, NeoPixel.GRB);  // Adjust the pin number and pixel count according to your setup

// Call neopixel_print_string
let neopixel_print_string = ffi('void neopixel_print_string_wrapper(char *, int, int)');

neopixel_print_string('ARR', 0, 0);

/*
let get_pixel_coordinates = ffi('int get_pixel_coordinates(char *, char *, int)');

let CHAR_WIDTH = 5; // Set the appropriate value for your custom font
let CHAR_HEIGHT = 8;
let SPACE_BETWEEN_CHARS = 1;

let text = "ABC";
let text_length = text.length;
let matrix_width = text_length * (CHAR_WIDTH + SPACE_BETWEEN_CHARS) - SPACE_BETWEEN_CHARS;
let matrix_height = CHAR_HEIGHT;

let coords_str = "";
let coords_size = 256; // You can adjust this value based on your needs
for (let i = 0; i < coords_size * 2; i++) {
  coords_str += "\x00";
}

let coords_count = get_pixel_coordinates(text, coords_str, coords_size);

let x_coords = [];
let y_coords = [];
for (let i = 0; i < coords_count; i++) {
  x_coords[i] = coords_str.at(i * 2);
  y_coords[i] = coords_str.at(i * 2 + 1);
}

// debug:
print('coords_count:', coords_count);
print('x_coords:', JSON.stringify(x_coords));
print('y_coords:', JSON.stringify(y_coords));


// Initialize an empty matrix with spaces
let matrix = [];
for (let y = 0; y < matrix_height; y++) {
  let row = [];
  for (let x = 0; x < matrix_width; x++) {
    row[x] = ' ';
  }
  matrix[y] = row;
}

// Fill the matrix with the pixel coordinates
for (let i = 0; i < coords_count; i++) {
  let x = x_coords[i];
  let y = y_coords[i];
  matrix[y][x] = '#';
}

// Print the matrix to the console
for (let y = 0; y < matrix_height; y++) {
  let row = '';
  for (let x = 0; x < matrix_width; x++) {
    row += matrix[y][x];
  }
  print(row);
}

// print('==================================');
// print('==================================');
// print('==================================');
// print('==================================');

// let print_string = ffi('void print_string(char *)');
// print_string('Hello, World!');
*/