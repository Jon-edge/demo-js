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
load('api_http.js');
load('api_rpc.js')

let btn = Cfg.get('board.btn1.pin');              // Built-in button GPIO
let led = Cfg.get('board.led1.pin');              // Built-in LED GPIO number
let onhi = Cfg.get('board.led1.active_high');     // LED on when high?
let state = {on: false, btcPrice: 0, btcDailyPct: 0, uptime: 0};  // Device state
let online = false;                               // Connected to the cloud?

let setLED = function(on) {
  let level = onhi ? on : !on;
  GPIO.write(led, level);
};

GPIO.set_mode(led, GPIO.MODE_OUTPUT);
setLED(state.on);

let reportState = function() {
  Shadow.update(0, state);
};

// Update state every second, and report to cloud if online
Timer.set(1000, Timer.REPEAT, function() {
  state.uptime = Sys.uptime();
  state.ram_free = Sys.free_ram();
  // print('online:', online, JSON.stringify(state));
  if (online) reportState();
}, null);

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

print('=============== INIT BEGIN ===================');
print('============= FFI Imports =====================');

let setString = ffi('void neopixel_set_string(char *, int, int, int, int, int)');
let clearPixels = ffi('void neopixel_clear()');
let showPixels = ffi('void neopixel_show()');
let randInt = ffi('int rand_int(int, int, int)');
// let round_to_precision = ffi('void round_to_precision(char *, double, int,
// int)');

// Initial tests
// printString('012345', 0, 9);

print('================ JS defs ==================');

let printString = function(str, x, y, opts) {
  x = x || 0;
  y = y || 0;
  let color = {r: 25, g: 25, b: 25};
  let isClear = true;
  let isShow = true;

  if (opts) {
    if (opts.color && opts.color.r !== undefined && opts.color.g !== undefined && opts.color.b !== undefined) {
      color = opts.color;
    }
    if (opts.isClear !== undefined) {
      isClear = opts.isClear;
    }
    if (opts.isShow !== undefined) {
      isShow = opts.isShow;
    }
  }
  
  if (isClear) {
    clearPixels();
  }

  setString(str, x, y, color.r, color.g, color.b);

  if (isShow) {
    showPixels();
  }
};

let roundToNearest = function(number) {
  let roundedNumber = (number >= 0) ? ((number * 100 + 0.5) | 0) : ((number * 100 - 0.5) | 0);
  return roundedNumber / 100;
};


let fetchBTCPrice = function() {
  if (online) {
  HTTP.query({
    url: 'https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true', 
    success: function (body, full_http_msg) {
      print('Fetching...');
      let response = JSON.parse(body);
      let btcPrice = JSON.stringify(roundToNearest(response.bitcoin.usd));
      let btcPctChangeRaw = response.bitcoin.usd_24h_change;
      // let btcPctChange = JSON.stringify(roundToPrecision(btcPctChangeRaw, 2));
      let btcPctChange = JSON.stringify(roundToNearest(btcPctChangeRaw)).slice(0, 4);
      print('BTC price:', btcPrice, 'USD');
      print('24h change:', btcPctChange, '%');
      state.btcPrice = btcPrice;
      state.btcDailyPct = btcPctChange;

      let pctSign = btcPctChangeRaw > 0 ? '+' : '';
      let color = (btcPctChangeRaw < 0) ? {r: 50, g: 0, b: 0} : {r: 0, g: 50, b: 0};

      printString(btcPrice, 1, 9, {isClear: true, isShow: false});
      
      let xOffsetHack = 1; // btcPctChange === '0' ? 5 : 1;
      printString(pctSign + btcPctChange + '%', xOffsetHack, 0, {isClear: false, isShow: true, color: color});

      print('Done.');
    },
    error: function(err) {
      print('Error fetching BTC price:', err);
    }
  });
  } else {
    printString("OFFLN", 0, 4)
  }
};

print('Drawing init matrix display');
printString('BRB', 6, 5);
print('=============== Init0 Done ===================');


// Fetch the BTC price every x seconds
Timer.set(15000, Timer.REPEAT, fetchBTCPrice, null);

// Heartbeat
Timer.set(1000, Timer.REPEAT, function(){
  state.on = !state.on;
  setLED(state.on);
}, null);


RPC.addHandler('Huddle', function(args) {
  if (typeof(args) === 'object' && typeof(args.count) === 'number') {
    let randNum = randInt(1, args.count, Sys.uptime());
    printString(JSON.stringify(randNum), 10, 5, {color: {r: 0, g: 75, b: 25}});
    return { value: randNum };
  } else {
    return {error: -1, message: 'Bad request. Expected: {"count":N1}'};
  }
});

print('=============== JS PARSING COMPLETE ===================');