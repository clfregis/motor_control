var temp = [], hum = [], time = [], state = [], daily = [], continuous = [];
var status;
var motor_label = document.getElementById("motorLabel").textContent;
var current_motor = 'Motor 1';
var yStatusLabels = {
	0 : 'Stopped',
  1 : 'Operating',
  2 : 'Stopped',
}

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
        time.push(snap[i][n]*1000);
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
  //temp = temp.slice(-12, temp.length);
  let current_hum = hum[hum.length - 1];
  //hum = hum.slice(-12, hum.length);
  let current_time = time[time.length - 1];
  //time = time.slice(-12, time.lenght);
  let current_state = state[state.length - 1];
  //state = state.slice(-12, state.lenght);
  let current_daily = daily[daily.length - 1];
  //daily = daily.slice(-12, daily.length);
  let current_continuous = continuous[continuous.length - 1];
  //continuous = continuous.slice(-12), daily.length;

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
      zoomTo: [local_xValues.length-15,local_xValues.length],
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
            'values': local_yValues,
            'line-color' : local_graphColor,
            'line-width' : 4,
            marker: { /* Marker object */
              'background-color': local_graphColor, /* hexadecimal or RGB value */
              size : 4, /* in pixels */
              'border-color' : local_graphColor, /* hexadecimal or RBG value */
              'border-width' : 2 /* in pixels */
            }
          }
        ]
  };
  zingchart.render({
    id: local_ID,
    data: chartConfig
  });

  // Zoom Button Magic
  // zingchart.loadModules('zoom-buttons', () => {
  //   // fetch the data remotely
  //   fetch('https://cdn.zingchart.com/datasets/timeseries-sample-data-2019.json')
  //     .then(res => res.json())
  //     .then(timeseriesData => {
  //       // assign data
  //       //chartConfig.series[0].values = timeseriesData.values;
  //       // destroy the chart since we have to render the
  //       // chart with a module. if there is no module,
  //       // just use set data like the catch statement
  //       zingchart.exec(local_ID, 'destroy');
  //       // render chart with width and height to
  //       // fill the parent container CSS dimensions
  //       zingchart.render({
  //         id: local_ID,
  //         data: chartConfig,
  //         modules: 'zoom-buttons'
  //       });
  //     })
  //     .catch(e => {
  //       // if error, render blank chart
  //       console.error('--- error fetching data from: https://cdn.zingchart.com/datasets/timeseries-sample-data.json ---');
  //       chartConfig.title = {};
  //       chartConfig.title.text = 'Error Fetching https://cdn.zingchart.com/datasets/timeseries-sample-data.json';
  //       // just exec setdata api method since we don't need to render the zoom modules
  //       // https://www.zingchart.com/docs/api/methods/
  //       zingchart.exec(local_ID, 'setdata', {
  //         data: chartConfig
  //       });
  //     });
  // });

}