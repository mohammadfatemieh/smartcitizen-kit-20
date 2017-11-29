var app = new Vue({
  el: '#app',
  data: {
    theApi: window.location.protocol + '//' + window.location.host + '/',
    development: false,
    browsertime: Math.floor(Date.now() / 1000),
    kitstatus: [],
    devicetime: 0,
    logging: [],
    intervals: false,
    kitinfo: false,
    currentPage: 0,
    lastPage: 0,
    page: [
       {'visible': true,  'footer': 'HOME',         'backTo': 0, 'back': false },
       {'visible': false, 'footer': 'REGISTER Key', 'backTo': 0, 'back': true  },
       {'visible': false, 'footer': 'REGISTER WiFi','backTo': 1, 'back': true  },
       {'visible': false, 'footer': 'Connecting',   'backTo': 0, 'back': false },
       {'visible': false, 'footer': 'Connecting',   'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'SD card',      'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'Kit info',     'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'Empty',        'backTo': 0, 'back': true },
    ],
    publishinterval: 2,
    readinginterval: 60,
    selectedWifi: '',
    sensors: false,
    sensor1: false,
    sensor2: false,
    sensor3: false,
    sensor4: false,
    showDebug: false,
    showExperimental: false,
    showInterval: false,
    showSdCard: false,
    sdlog: false,
    usertoken: '',
    version: 'SCK 2.0 / SAM V0.0.2 / ESP V0.0.2',
    wifiname: '',
    wifipass: '',
    wifisync: true,
    wifis: {
      "nets": [{
        "ssid": "Fake-WIFI-1",
        "ch": 1,
        "rssi": -64
      }, {
        "ssid": "Fake-Wifi-2",
        "ch": 1,
        "rssi": -89
      }]
    }
  },
  mounted: function () {
    // When the app is mounted
    this.logging.push('App started.');

    this.checkIfDebugMode;

    // 1. Remove loading screen
    var el = document.getElementById('loading');
    el.parentNode.removeChild(el);

    var el1 = document.getElementById('app');
    el1.style.display = 'block';

    // 2. Select which API to use, dev vs prod
    this.selectApiUrl();

    // 3. Fetch available Wifis + status
    this.jsGet('aplist');
    this.jsGet('status');

    // This checks if connection to the kit has been lost, every X sec
    this.periodic(9000);
  },
  methods: {
    periodic: function (ms) {
      var that = this;

      // TODO: should we check status every 5 sec?
      setTimeout(function(){
        that.jsGet('status');
        return that.periodic(ms);
      }, ms);
    },
    selectApiUrl: function () {
      // If we are running this from the kit,
      // the API should be on the same IP and port
      // Most likely a 192.168.*.1/status

      // If we are using port 8000, we are in development, and the API should be on port 3000
      if (window.location.port === '8000') {
        this.theApi = 'http://' + window.location.hostname + ':3000/';
        this.development = true;
      }

      //console.log('Using API : ' + this.theApi);
      this.notify('Using API', 1000);
    },
    httpGet: function(theUrl, callback) {
      //console.log('theurl: ' + theUrl);
      var xmlHttp = new XMLHttpRequest();
      var that = this;

      xmlHttp.onreadystatechange =  function() {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
          callback(xmlHttp.responseText);
        }
      }
      xmlHttp.onerror = function(e){
        that.notify('Cannot access API', 5000, 'bg-red');
      }
      xmlHttp.open( "GET", theUrl, true ); // false for synchronous request, true = async
      xmlHttp.send( null );
    },
    jsGet: function(path) {
      var that = this;

      this.httpGet(this.theApi + path, function(res){
        //console.log('Getting: ' + path);
        //console.log(JSON.parse(res));
        if (path === 'aplist') {
          that.wifis = JSON.parse(res);
          that.notify('Getting wifi list...', 1000, 'bg-cyan');
        }
        if (path === 'status'){
          //that.notify('Getting status', 1000);
          that.kitstatus = JSON.parse(res);
        }

      });

    },
    // Not actually a POST request, but a GET with params
    jsPost: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      var that = this;
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value

      if (purpose == 'connect'){
        this.notify('Connecting online...', 2000);
        that.httpGet(that.theApi + path +
            '?ssid=' + that.selectedWifi +
            '&password=' + that.wifipass +
            '&token=' + that.usertoken +
            '&epoch=' + that.browsertime,
            function(res){
              console.log('Kit response: ' + res);
            });
        // After we try to connect, we go to the next page.
        this.gotoPage();
      }

      if (purpose == 'synctime'){
        this.notify('Starting to log on SD CARD..', 2000);
        this.httpGet(this.theApi +  path + '?epoch=' + this.browsertime, function(res){
          // TODO: What is the correct response.key from the kit?
          that.notify(JSON.parse(res).todo, 5000);
        });
      }

    },

    gotoPage: function(num){

      // Keep lastPage in memory
      this.lastPage = this.currentPage;

      // Hide current page
      this.page[this.currentPage].visible = false;

      // Find which page to show next
      // Is it a specified page, or the next one?
      if ( typeof num !== 'undefined') {
        this.currentPage = parseInt(num);
      }else{
        // Find the last page so we wont go too far, when clicking 'Next'
        if ( this.currentPage === (this.page.length - 1)) {
          //console.log('Last page: ' + this.currentPage)
          return;
        }

        this.currentPage = parseInt(this.currentPage + 1);
      }

      // Show it
      this.page[this.currentPage].visible = true;

      this.logging.push('Go to page: ' + this.currentPage);
    },

    notify: function(msg, duration, className){
      if (duration === 'undefined') {
        console.log('no duration');
        duration = 1000;
      }

      //All events should also go to the logging section
      this.logging.push(msg);

      var newtoast = document.createElement("div");
      if (className) {
        newtoast.className = "toast " + className;
      }else{
        newtoast.className = "toast";
      }
      newtoast.innerHTML = msg;
      document.getElementById("toast-wrapper").appendChild(newtoast);

      setTimeout(function(){
        newtoast.outerHTML = "";
        delete newtoast;
      }, duration);

      console.log('Notify:', msg);
    },
  },
  computed: {
    checkIfDebugMode: function(){
       if (window.location.hash === '#debug'){
         // Enables debug buttons
         this.showDebug = true;
       }
    },
    sortedWifi: function () {
      this.wifis.nets.sort(function (a, b) {
        return a.rssi - b.rssi;
      });
      return this.wifis.nets.reverse();
    },
    usertokenCheck: function(){
      return this.usertoken.length === 6;
    }
  }
});
