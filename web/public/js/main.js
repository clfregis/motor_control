// Paste below the Firebase config object
var firebaseConfig = {
  apiKey: "AIzaSyD0La4jfHL-3QPhO8djiHeCgrj6uM4T5_s",
  authDomain: "margarita-1ddc1.firebaseapp.com",
  databaseURL: "https://margarita-1ddc1.firebaseio.com",
  projectId: "margarita-1ddc1",
  storageBucket: "margarita-1ddc1.appspot.com",
  messagingSenderId: "98433319312",
  appId: "1:98433319312:web:3a540923827bfd31663315"
};

var temp = [],gas = [], hum = [], time = [], state = [], daily = [], continuous = [], timeDaily = [];
var status, current_motor;
const currentMotor = {
  'motor_1' : 'Motor 1',
  'motor_2' : 'Motor 2',
  'motor_3' : 'Motor 3',
  'motor_4' : 'Motor 4'
}
const yStatusLabels = {
	0 : 'Stopped',
  1 : 'Operating',
  2 : 'Halted'
};

// Update the motor label accordingly to the page loaded
var motor_label = document.getElementById("motorLabel").textContent;


// Initialize Firebase
firebase.initializeApp(firebaseConfig);


// window.onload event for Javascript to run after HTML
// because this Javascript is injected into the document head
window.addEventListener('load', () => {
  // Get a reference to the database service
  var database = firebase.database().ref(motor_label);
  //firebase.database.enableLogging(true);
  // Read the database continuously, capture the values, update page and draw graphs
  database.on('value', function(snapshot) {
    let snap = snapshot.val(); // vai ter todo o objeto
    for (i in snap){ // vai iterar em cada key
      for (n in snap[i]){
        if (n=='T'){
          temp.push(snap[i][n]);
        }
        if (n=='H'){
          hum.push(snap[i][n]);
        }
        if (n=='D'){
          time.push(snap[i][n]*1000); //Multiply by thousand, because Zingchart works with ms, instead of sec
        }
        if (n=='S'){
          state.push(snap[i][n]);
        }
        if (n=='DO'){
          daily.push(format_ISOhour(snap[i][n]));
        }
        if (n=='CO'){
          continuous.push(format_ISOhour(snap[i][n]));
        }
        if (n=='GAS'){
          gas.push(snap[i][n]);
        }
      }
    }
    let current_temp = temp[temp.length - 1];
    let current_hum = hum[hum.length - 1];
    let current_time = time[time.length - 1];
    let current_state = state[state.length - 1];
    let current_daily = daily[daily.length - 1];
    let current_continuous = continuous[continuous.length - 1];
    let current_gas = gas[gas.length - 1];
    // Update variables to a human readable format
    status = yStatusLabels[current_state];
    current_motor = currentMotor[motor_label];
    // Update page elements with values retrieved from firebase
    const timestamp = new Date().getTime();
    if((timestamp-current_time)>=(10*60*1000)){
      document.getElementById("status").innerHTML = "Status: Offline";
    }
    else{
      document.getElementById("status").innerHTML = "Status: " + status;
    }
    document.getElementById("nameMotor").innerHTML = "Name: " + current_motor;
    document.getElementById("currentTemperature").innerHTML = "Temperature: " + current_temp + "ºC";
    document.getElementById("currentHumidity").innerHTML = "Humidity: " + current_hum + "%";
    document.getElementById("dailyOperation").innerHTML = "Daily: " + current_daily;
    document.getElementById("continuousOperation").innerHTML = "Continuous: " + current_continuous;
    document.getElementById("gasValue").innerHTML = "Gas: " + current_gas;
    document.getElementById("lastUpdate").innerHTML = "Last Update: " + format_hour(current_time/1000) + ", " + format_date(current_time/1000);
    // Draw Graphs
    drawGraph(time, temp, 'Temperature [ºC]', 'Temperature', 'Temperature: %v ºC', 'spline', '#C6BD74', 45);
    drawGraph(time, hum, 'Humidity [%]', 'Humidity', 'Humidity: %v %', 'spline', '#84A6CC', 100);
    drawGraphStatus(time, state, 'Operation', 'statusChart', 'Status: %v', 'stepped', '#689167', 2);
  });
  // Update the SP time of the current motor
  var databaseSP = firebase.database().ref('sp_time/'+motor_label);
  databaseSP.once('value', function(snapshot){
    let currentSPTime = snapshot.val();
    document.getElementById("currentSP").innerHTML = "Current SP time: " + currentSPTime + " s";
  });

  // Update the bar graph the current motor
  var databaseBar = firebase.database().ref('daily/'+motor_label);
  databaseBar.once('value', function(snapshot){
    let snapBar = snapshot.val();
    let dailyOperationTime = [];
    for (i in snapBar){
      for (n in snapBar[i]){
        if (n=='D'){
          timeDaily.push(snapBar[i][n]*1000); //Multiply by thousand, because Zingchart works with ms, instead of sec
        }
        if (n=='DO'){
          dailyOperationTime.push(snapBar[i][n]);
        }
      }
    }
    drawBarGraph(timeDaily, dailyOperationTime, 'Daily Operation Time [sec]', 'dailyStatusChart','#E79399', 1000, 'Daily Operation Time: %v s');
    
  });

  // Store the value typed on input
  var letSPTime = document.getElementById('SPTime');
  // Set this value on the database upon clicking
  document.getElementById("changeSPTime").onclick = () => { 
    if (letSPTime.value>65){
      databaseSP.set(parseInt(letSPTime.value));
      location.reload();
    }
  };

  function getDataFromServer(callback){
    if(typeof callback === 'function'){
      setInterval(function(){
        callback();
    },1000)
    }
  }

  // If the motor is halted, then the reset button has effect
  function updateFrontEndStatus (){
    var databaseStatus = firebase.database().ref('front_end_reset_status/'+motor_label);

    if(status=='Halted'){
      document.getElementById("resetButton").onclick = () => { 
        databaseStatus.set(1);
        // Loading
        document.getElementById("loader").style.display = "block";
      };
    }
    if(status=='Operating' || status=='Stopped'){
      // Stop loading
      document.getElementById("loader").style.display = "none";
      databaseStatus.set(0);
    }
  }

  getDataFromServer(updateFrontEndStatus);
  
});

function format_hour(seconds){
  // https://www.geeksforgeeks.org/javascript-date-toisostring-function/
  var date = new Date(null);
  date.setSeconds(seconds); // specify value for SECONDS here, from ESP32, UTC time
  var result = date.toString().substr(16, 8); //toSring automatically adjust the TimeZone
  return result;
}

function format_ISOhour(seconds){
  // https://www.geeksforgeeks.org/javascript-date-toisostring-function/
  var date = new Date(null);
  date.setSeconds(seconds); // specify value for SECONDS here, from ESP32, delta time, no TIMEZONE required
  var result = date.toISOString().substr(11, 8);
  return result;
}

function format_date(seconds){
  // https://www.w3schools.com/js/js_dates.asp
  var date = new Date(null);
  date.setSeconds(seconds); // specify value for SECONDS here, from ESP32, UTC time
  var result = date.toDateString(); //toSring automatically adjust the TimeZone
  return result;
}

function drawGraph(local_xValues, local_yValues, local_yLabel, local_ID, 
  local_tooltipText, local_aspect, local_graphColor, yMaxValue) {
  const chartConfig = {
    type: 'line',
    globals: {
      backgroundColor: 'transparent',
      fontFamily: 'Helvetica Neue',
      fontColor: '#BBC1CB',
    },
    plot: {
      aspect: local_aspect
    },
    plotarea: {
      marginTop: 10,
      marginLeft: 60,
      marginRight: 60,
      marginBottom: 120,
    },
    scaleY: {
      maxValue: yMaxValue,
      minValue: 0,
      step: 1,
      tick: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      label: {
        text: local_yLabel,
        bold: false,
        fontSize: 18,
      },
      zooming:false
    },

    crosshairX: {
      lineColor: '#BBC1CB',
      lineStyle: 'dashed',
      lineWidth: '1px',
      marker: {
        type: 'triangle',
        size: '5px',
        visible: true
      },
      plotLabel: {
        visible: false
      },
      scaleLabel:{
        backgroundColor: '#484F5D',
        color: '#C6BD74',
        borderColor: '#BBC1CB',
        borderWidth: '1px',
        fontSize: 14,
      },
    },

    scaleX: {
      labels: local_xValues,
      step: "second",
      transform: {
        type: 'date',
        all: '%D, %dd %M %Y<br>%H:%i:%s',
      },
      tick: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      zooming: true,
      zoomTo: [local_xValues.length-30,local_xValues.length],
    },
    tooltip: {
      backgroundColor: '#484F5D',
      color: '#C6BD74',
      fontSize: 14,
      text: local_tooltipText,
    },
    zoom:{
      backgroundColor: '#C6BD74',
      borderColor: '#BBC1CB',
      borderWidth: 1,
    },

    preview: {
      mask:{
        backgroundColor: 'white',
      },
      handle: {
        height: 30,
        backgroundColor: 'white',
      },
      label: {
        visible: false,
      },
      live: true,
      
    },
    noData: {
      text: 'No data found',
      backgroundColor: 'transparent'
    },
    series: [
          { 
            values: local_yValues,
            lineColor : local_graphColor,
            lineWidth : 4,
            marker: { /* Marker object */
              backgroundColor: local_graphColor, /* hexadecimal or RGB value */
              size : 4, /* in pixels */
              borderColor : local_graphColor, /* hexadecimal or RBG value */
              borderWidth : 2 /* in pixels */
            }
          }
        ]
  };
  zingchart.render({
    id: local_ID,
    data: chartConfig
  });

}

function drawGraphStatus(local_xValues, local_yValues, local_yLabel, local_ID, 
  local_tooltipText, local_aspect, local_graphColor, yMaxValue) {
  const chartConfig = {
    type: 'line',
    globals: {
      backgroundColor: 'transparent',
      fontFamily: 'Helvetica Neue',
      fontColor: '#BBC1CB',
    },
    plot: {
      aspect: local_aspect
    },
    plotarea: {
      marginTop: 10,
      marginLeft: 75,
      marginRight: 60,
      marginBottom: 120,
    },
    scaleY: {
      labels: [
        'Off',
        'On',
        'Halted'
      ],
      maxValue: yMaxValue,
      minValue: 0,
      step: 1,
      tick: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      label: {
        text: local_yLabel,
        bold: false,
        fontSize: 18,
      },
      zooming:false
    },

    crosshairX: {
      lineColor: '#BBC1CB',
      lineStyle: 'dashed',
      lineWidth: '1px',
      marker: {
        type: 'triangle',
        size: '5px',
        visible: true
      },
      plotLabel: {
        visible: false
      },
      scaleLabel:{
        backgroundColor: '#484F5D',
        color: '#C6BD74',
        borderColor: '#BBC1CB',
        borderWidth: '1px',
        fontSize: 14,
      },
    },

    scaleX: {
      labels: local_xValues,
      step: "second",
      transform: {
        type: 'date',
        all: '%D, %dd %M %Y<br>%H:%i:%s',
      },
      tick: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      zooming: true,
      zoomTo: [local_xValues.length-30,local_xValues.length],
    },
    tooltip: {
      backgroundColor: '#484F5D',
      color: '#C6BD74',
      fontSize: 14,
      text: local_tooltipText,
    },
    zoom:{
      backgroundColor: '#C6BD74',
      borderColor: '#BBC1CB',
      borderWidth: 1,
    },

    preview: {
      mask:{
        backgroundColor: 'white',
      },
      handle: {
        height: 30,
        backgroundColor: 'white',
      },
      label: {
        visible: false,
      },
      live: true,
      
    },
    noData: {
      text: 'No data found',
      backgroundColor: 'transparent'
    },
    series: [
          { 
            values: local_yValues,
            lineColor : local_graphColor,
            lineWidth : 4,
            marker: { /* Marker object */
              backgroundColor: local_graphColor, /* hexadecimal or RGB value */
              size : 4, /* in pixels */
              borderColor : local_graphColor, /* hexadecimal or RBG value */
              borderWidth : 2 /* in pixels */
            }
          }
        ]
  };
  zingchart.render({
    id: local_ID,
    data: chartConfig
  });

}

function drawBarGraph(local_xValues, local_yValues, local_yLabel, local_ID, local_graphColor, yMaxValue, local_tooltipText) {
  const chartConfig = {
    type: 'bar',
    globals: {
      backgroundColor: 'transparent',
      fontFamily: 'Helvetica Neue',
      fontColor: '#BBC1CB',
    },
    plotarea: {
      marginTop: 10,
      marginLeft: 70,
      marginRight: 60,
      marginBottom: 120,
    },
    scaleY: {
      maxValue: yMaxValue,
      minValue: 0,
      // step: 1,
      tick: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      label: {
        text: local_yLabel,
        bold: false,
        fontSize: 18,
      },
      zooming:false
    },

    scaleX: {
      labels: local_xValues,
      step: "day",
      transform: {
        type: 'date',
        all: '%D, %dd %M %Y<br>%H:%i:%s',
      },
      guide: {
        lineColor: '#676E7A',
        lineStyle: 'solid',
        lineWidth: '1px',
        visible: true,
        rules: [{
          rule: "%i == 0",
          visible: false
        }]
      },
      item: {
        fontSize: 14,
      },
      zooming: true,
      zoomTo: [local_xValues.length-30,local_xValues.length],
    },
    tooltip: {
      backgroundColor: '#484F5D',
      color: '#C6BD74',
      fontSize: 14,
      text: local_tooltipText,
    },
    zoom:{
      backgroundColor: '#C6BD74',
      borderColor: '#BBC1CB',
      borderWidth: 1,
    },

    preview: {
      mask:{
        backgroundColor: 'white',
      },
      handle: {
        height: 30,
        backgroundColor: 'white',
      },
      label: {
        visible: false,
      },
      live: true,
      
    },
    noData: {
      text: 'No data found',
      backgroundColor: 'transparent'
    },
    series: [
          { 
            values: local_yValues,
            backgroundColor: local_graphColor, /* hexadecimal or RGB value */
          }
        ]
  };
  zingchart.render({
    id: local_ID,
    data: chartConfig
  });

}


// /**
//  * As we are moving the guide, we want to update the items outside
//  * of the chart. We could do this inside the chart, but since JS
//  * is single threaded we don't want to block UI with a chart 
//  * update. A chart update is more expensive than a node update.
//  * You may see no performance gains on a dataset this size, but
//  * with some increase its possible to see a discrepancy for the
//  * user. This is why I chose to contstruct the items outside
//  * of the graph.
//  */
// // zingchart.guide_mousemove = (e) => {
// //   document.getElementById('day').innerHTML = 'Day: ' + e['scale-label']['scale-x'].substr(0,16);
// // }

// /** 
//  * ZingChart defined event listener. Will capture ZoomEvent and related 
//  * Zooming Information.
//  */
// zingchart.zoom = (e) => {
//   displayZoomValues(format_date(e.kmin/1000), format_date(e.kmax/1000));
// }
 
 
/**
 * Apply dates to display current zoomed dates
 */
// let displayZoomValues = (sFrom, sTo) => {
//   let dateFrom = document.getElementById('date-from');
//   let dateTo = document.getElementById('date-to');
 
//   // If viewall is clicked show the default dates
//   if (!sFrom) {
//     sFrom = '27/04/2020';
//   }
//   if (!sTo) {
//     sTo = '28/04/2020';
//   }
 
//   dateFrom.innerHTML = sFrom;
//   dateTo.innerHTML = sTo;
// }
 

/**
 * Apply zoom to graph.
 */
let zoomToIndex = (minValue, maxValue, local_ID) => {
 
  // ZingChart api automated zoom. Have to be careful
  // not to zoom past the end of the graph
  zingchart.exec(local_ID, 'zoomto', {
    xmax: maxValue,
    xmin: minValue,
  });
}

$('#datepicker_temperature').datepicker({ 
    dateFormat: 'dd/mm/yy',
    showOtherMonths: true,
    showOtherMonths: true }).on("change", function() {
      // Get date from input form
      var datePicker = document.getElementById("datepicker_temperature").value;
      // Structure date
      var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
      // Create a date variable
      var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
      // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
      // Turns out that it is not necessary to subtract the value since when returning to front end
      // in zoom function, it does the timezone again
      var transformedDate = Date.parse(d)/*-10800000*/;
      var minmin = function() {
        for (var i=0; i<time.length;i++){
          if (time[i]>=transformedDate){
            return i;
          }
        }
      }();

      var maxmax = function() {
        for (var i=0; i<time.length;i++){
          if (time[i]>=(transformedDate+(24*3600*1000))){
            return i;
          }
        }
      }();

      zoomToIndex(minmin, maxmax, 'Temperature');
    });

$('#datepicker_humidity').datepicker({ 
  dateFormat: 'dd/mm/yy',
  showOtherMonths: true,
  showOtherMonths: true }).on("change", function() {
    // Get date from input form
    var datePicker = document.getElementById('datepicker_humidity').value;
    // Structure date
    var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
    // Create a date variable
    var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
    // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
    // Turns out that it is not necessary to subtract the value since when returning to front end
    // in zoom function, it does the timezone again
    var transformedDate = Date.parse(d)/*-10800000*/;
    var minmin = function() {
      for (var i=0; i<time.length;i++){
        if (time[i]>=transformedDate){
          return i;
        }
      }
    }();

    var maxmax = function() {
      for (var i=0; i<time.length;i++){
        if (time[i]>=(transformedDate+(24*3600*1000))){
          return i;
        }
      }
    }();

    zoomToIndex(minmin, maxmax, 'Humidity');
  });

$('#datepicker_status').datepicker({ 
  dateFormat: 'dd/mm/yy',
  showOtherMonths: true,
  showOtherMonths: true }).on("change", function() {
    // Get date from input form
    var datePicker = document.getElementById("datepicker_status").value;
    // Structure date
    var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
    // Create a date variable
    var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
    // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
    // Turns out that it is not necessary to subtract the value since when returning to front end
    // in zoom function, it does the timezone again
    var transformedDate = Date.parse(d)/*-10800000*/;
    var minmin = function() {
      for (var i=0; i<time.length;i++){
        if (time[i]>=transformedDate){
          return i;
        }
      }
    }();

    var maxmax = function() {
      for (var i=0; i<time.length;i++){
        if (time[i]>=(transformedDate+(24*3600*1000))){
          return i;
        }
      }
    }();

    zoomToIndex(minmin, maxmax, 'statusChart');
  });

  $('#datepicker_dailyStatus').datepicker({ 
    dateFormat: 'dd/mm/yy',
    showOtherMonths: true,
    showOtherMonths: true }).on("change", function() {
      // Get date from input form
      var datePicker = document.getElementById("datepicker_dailyStatus").value;
      // Structure date
      var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
      // Create a date variable
      var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
      // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
      // Turns out that it is not necessary to subtract the value since when returning to front end
      // in zoom function, it does the timezone again
      var transformedDate = Date.parse(d)/*-10800000*/;
      var minmin = function() {
        for (var i=0; i<timeDaily.length;i++){
          if (timeDaily[i]>=transformedDate){
            return i;
          }
        }
      }();
  
      var maxmax = function() {
        for (var i=0; i<timeDaily.length;i++){
          if (timeDaily[i]>=(transformedDate+(3*24*3600*1000))){
            return i;
          }
        }
      }();

      console.log(minmin);
      console.log(maxmax);
  
      zoomToIndex(minmin, maxmax, 'dailyStatusChart');
    });