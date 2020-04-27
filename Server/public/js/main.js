var temp = [], hum = [], time = [], state = [], xlabel = [], daily = [], continuous = [];
var status = 'OFF';
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

firebase.database.enableLogging(true);

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
        const tmp1 = snap[i][n].split(' ');
        const tmp2 = snap[i][n].split('');
        xlabel.push(tmp1[0]+", "+tmp1[1]+" "+tmp1[2]+" "+tmp1[4]);
        time.push(tmp2[11]+tmp2[12]+tmp2[13]+tmp2[14]+tmp2[15]);
      }
      if (n=='S'){
        state.push(snap[i][n]);
      }
      if (n=='DO'){
        daily.push(format_hour(snap[i][n]));
      }
      if (n=='CO'){
        continuous.push(format_hour(snap[i][n]));
      }
    }
  }
  let current_temp = temp[temp.length - 1];
  temp = temp.slice(-12, temp.length);
  let current_hum = hum[hum.length - 1];
  hum = hum.slice(-12, hum.length);
  let current_time = time[time.length - 1];
  time = time.slice(-12, time.lenght);
  let current_state = state[state.length - 1];
  state = state.slice(-12, state.lenght);
  let current_xlabel = xlabel[xlabel.length - 1];
  xlabel = xlabel.slice(-12, xlabel.length);
  let current_daily = daily[daily.length - 1];
  daily = daily.slice(-12, daily.length);
  let current_continuous = continuous[continuous.length - 1];
  continuous = continuous.slice(-12), daily.length;

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

  document.getElementById("status").innerHTML = "Status: " + status;
  document.getElementById("nameMotor").innerHTML = "Name: " + current_motor;
  document.getElementById("currentTemperature").innerHTML = "Temperature: " + current_temp + "ºC";
  document.getElementById("currentHumidity").innerHTML = "Humidity: " + current_hum + "%";
  document.getElementById("dailyOperation").innerHTML = "Daily: " + current_daily;
  document.getElementById("continuousOperation").innerHTML = "Continuous: " + current_continuous;
  document.getElementById("lastUpdate").innerHTML = "Last Update: " + current_time + ", " + current_xlabel;

  drawTemperature(time, current_xlabel, temp);
  drawHumidity(time, current_xlabel, hum);
  drawGraphStatus(time, current_xlabel, state);
});

// function changeMotor(newMotor){
//   document.getElementById("motorLabel").innerHTML = newMotor;
// }

function format_hour(seconds){
  var date = new Date(null);
  date.setSeconds(seconds); // specify value for SECONDS here
  var result = date.toISOString().substr(11, 8);
  return result;
}

function drawTemperature(label, label_title, graph) {
  var ctx = document.getElementById("Temperature").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Temperature",
        labelString: "ºC",
        borderColor: "#C6BD74",
        backgroundColor: "#C6BD74",
        borderWidth: 4,
        fill: false,
        data: graph,
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      aspectRatio: 1,
      hoverMode: 'index',
      stacked: false,
      title: {
        display: false,
      },
      legend: {
        display: false,
      },

      scales: {
        xAxes: [{
          display: true,
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          },
          scaleLabel: {
            display: true,
            labelString: label_title,
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
        yAxes: [{
          type: "linear",
          display: true,
          position: "left",
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            beginAtZero: true,
            stepSize: 1,
            Max: 50,
            suggestedMax: 45,
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          },
          scaleLabel: {
            display: true,
            labelString: 'ºC',
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
      }
    }
  });
}

function drawHumidity(label, label_title, graph) {
  var ctx = document.getElementById("Humidity").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Humidity",
        labelString: "%",
        borderColor: "#84A6CC",
        backgroundColor: "#84A6CC",
        borderWidth: 4,
        fill: false,
        data: graph,
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      aspectRatio: 1,
      hoverMode: 'index',
      stacked: false,
      title: {
        display: false,
      },
      legend: {
        display: false,
      },

      scales: {
        xAxes: [{
          display: true,
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          },
          scaleLabel: {
            display: true,
            labelString: label_title,
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
        yAxes: [{
          type: "linear",
          display: true,
          position: "left",
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            beginAtZero: true,
            stepSize: 1,
            Max: 100,
            suggestedMax: 100,
            Min: 0,
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          },
          scaleLabel: {
            display: true,
            labelString: '%',
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
      }
    }
  });
}
  

function drawGraphStatus(label, label_title, graph1) {
  var ctx = document.getElementById("StatusChart").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Status",
        borderColor: '#689167',
        backgroundColor: '#689167',
        borderWidth: 4,
        lineTension: 0,
        fill: false,
        data: graph1,
        steppedLine: true,
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      aspectRatio: 1,
      hoverMode: 'index',
      stacked: false,
      title: {
        display: false,
      },
      legend: {
        display: false,
      },

      scales: {
        xAxes: [{
          display: true,
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          },
          scaleLabel: {
            display: true,
            labelString: label_title,
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
        yAxes: [{
          type: "linear",
          display: true,
          position: "left",
          gridLines: {
            display: true,
            color: '#676E7A',
            zeroLineColor: '#676E7A',
          },
          ticks: {
            beginAtZero: true,
            stepSize: 1,
            Max: 1,
            fontSize: 14,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
            callback: function(value, index, values) {
              return yStatusLabels[value];
            }
          },
          scaleLabel: {
            display: false
          }
        }],
      }
    }
  });
}