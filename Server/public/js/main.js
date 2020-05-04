var temp = [], hum = [], time = [], state = [], daily = [], continuous = [];
var status;
var current_motor = 'Motor 1';
const yStatusLabels = {
	0 : 'Stopped',
  1 : 'Operating',
  2 : 'Stopped',
};

var motor_label = document.getElementById("motorLabel").textContent;

var firebaseConfig = {
    apiKey: "AIzaSyA-fPrOStfPGq-Ze_wW3mloB-Qo2AhgU8c",
    authDomain: "esp32-66ba5.firebaseapp.com",
    databaseURL: "https://esp32-66ba5.firebaseio.com",
    projectId: "esp32-66ba5",
    storageBucket: "esp32-66ba5.appspot.com",
    messagingSenderId: "502036978763",
    appId: "1:502036978763:web:5691be205391772c2a01ce",
    measurementId: "G-85ZFMD2ZK8"
  };
// Initialize Firebase
firebase.initializeApp(firebaseConfig);


// window.onload event for Javascript to run after HTML
// because this Javascript is injected into the document head
window.addEventListener('load', () => {
  // Get a reference to the database service
  var database = firebase.database().ref(motor_label);
  //firebase.database.enableLogging(true);

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
      }
    }
    let current_temp = temp[temp.length - 1];
    let current_hum = hum[hum.length - 1];
    let current_time = time[time.length - 1];
    let current_state = state[state.length - 1];
    let current_daily = daily[daily.length - 1];
    let current_continuous = continuous[continuous.length - 1];

    if (current_state==1){
      status='Operating';
    }
    else if (current_state==0){
      status='Stopped';
    }
    else{
      status='Halted';
    }

    if (motor_label=='motor_1'){
      current_motor = 'Motor 1';
    }
    else if (motor_label=='motor_2'){
      current_motor = 'Motor 2';
    }
    else if (motor_label=='motor_3'){
      current_motor = 'Motor 3';
    }
    else if (motor_label=='motor_4'){
      current_motor = 'Motor 4';
    }

    document.getElementById("status").innerHTML = "Status: " + status;
    document.getElementById("nameMotor").innerHTML = "Name: " + current_motor;
    document.getElementById("currentTemperature").innerHTML = "Temperature: " + current_temp + "ºC";
    document.getElementById("currentHumidity").innerHTML = "Humidity: " + current_hum + "%";
    document.getElementById("dailyOperation").innerHTML = "Daily: " + current_daily;
    document.getElementById("continuousOperation").innerHTML = "Continuous: " + current_continuous;
    document.getElementById("lastUpdate").innerHTML = "Last Update: " + format_hour(current_time/1000) + ", " + format_date(current_time/1000);

    drawGraph(time, temp, 'Temperature [ºC]', 'Temperature', 'Temperature: %v ºC', 'spline', '#C6BD74', 45);
    drawGraph(time, hum, 'Humidity [%]', 'Humidity', 'Humidity: %v %', 'spline', '#84A6CC', 100);
    drawGraph(time, state, 'Operation', 'statusChart', 'Status: %v', 'stepped', '#689167', 1);
    // drawGraphMatrix(time, matrixTemp, 'Temperature [ºC]', 'noCu', 'Temperature: %v ºC', 'spline', '#C6BD74', 45);
  });
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
//   console.log(e);
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
 
// document.getElementById("datepicker").addEventListener("input", () => {
//   console.log('Vai tomar no cu, anda logo!');
// });

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
      console.log(datePicker);
    
      // Structure date
      var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
      console.log(structuredDatePicker);
    
      // Create a date variable
      var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
      console.log(d);
    
      // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
      // Turns out that it is not necessary to subtract the value since when returning to front end
      // in zoom function, it does the timezone again
      var transformedDate = Date.parse(d)/*-10800000*/;
      console.log(transformedDate);
    
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
    console.log(datePicker);
  
    // Structure date
    var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
    console.log(structuredDatePicker);
  
    // Create a date variable
    var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
    console.log(d);
  
    // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
    // Turns out that it is not necessary to subtract the value since when returning to front end
    // in zoom function, it does the timezone again
    var transformedDate = Date.parse(d)/*-10800000*/;
    console.log(transformedDate);
  
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
    console.log(datePicker);
  
    // Structure date
    var structuredDatePicker = [datePicker.substr(0,2),datePicker.substr(3,2),datePicker.substr(6,4)];
    console.log(structuredDatePicker);
  
    // Create a date variable
    var d = new Date(structuredDatePicker[2],structuredDatePicker[1]-1,structuredDatePicker[0]);
    console.log(d);
  
    // Transform its value to UNIX timestamp, if Brazil, GMT-3, subtracts 3*3600*1000
    // Turns out that it is not necessary to subtract the value since when returning to front end
    // in zoom function, it does the timezone again
    var transformedDate = Date.parse(d)/*-10800000*/;
    console.log(transformedDate);
  
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