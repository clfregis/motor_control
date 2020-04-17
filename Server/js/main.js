var temp = [], hum = [], time = [], state = [];
var status = 'OFF';


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
var database = firebase.database().ref("motor_2");
database.on('value', function(snapshot) {
  let snap = snapshot.val(); // vai ter todo o objeto
  for (i in snap){ // vai iterar em cada key
    for (n in snap[i]){
      if (n=='Temperature'){
        temp.push(snap[i][n]);
      }
      if (n=='Humidity'){
        hum.push(snap[i][n]);
      }
      if (n=='Time'){
        time.push(snap[i][n]);
      }
      if (n=='Status'){
        state.push(snap[i][n]);
      }
    }
  }
  temp = temp.slice(-10, temp.length);
  hum = hum.slice(-10, hum.length);
  time = time.slice(-10, time.lenght);
  state = state.slice(-10, state.lenght);

  if (state[9]==1){
    status='ON';
  }
  else{
    status='OFF';
  }

  document.getElementById("status").innerHTML = "Status: " + status;

  drawGraphWeather(time, temp, hum);
  drawGraphStatus(time, state);
});

function drawGraphWeather(label, graph1, graph2) {
  var ctx = document.getElementById("WeatherChart").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Temperature",
        labelString: "ºC",
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgb(255, 99, 132)',
        fill: false,
        data: graph1,
        yAxisID: "y-axis-heartbeat",
      },
      {
        label: "Humidity",
        labelString: "%",

        borderColor: 'rgb(0, 99, 132)',
        backgroundColor: 'rgb(0, 99, 132)',
        fill: false,
        data: graph2,
        yAxisID: "y-axis-heartbeat",

      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      hoverMode: 'index',
      stacked: false,
      title: {
        display: true,
        text: 'Weather Station'
      },

      scales: {
        yAxes: [{
          type: "linear",
          display: true,
          position: "left",
          id: "y-axis-heartbeat",
          ticks: {
            beginAtZero: true,
            suggestedMax: 50
          }

        }],
      }
    }
  });
}



function drawGraphStatus(label, graph1) {
  var ctx = document.getElementById("StatusChart").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Status",

        borderColor: 'rgb(0, 99, 132)',
        backgroundColor: 'rgb(0, 99, 132)',
        lineTension: 0,
        fill: false,
        data: graph1,
        yAxisID: "y-axis-heartbeat",

      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      hoverMode: 'index',
      stacked: false,
      title: {
        display: true,
        text: 'Operation'
      },

      scales: {
        yAxes: [{
          type: "linear",
          display: true,
          position: "left",
          id: "y-axis-heartbeat",
          ticks: {
            beginAtZero: true,
            stepSize: 1,
            Max: 1
          }

        }],
      }
    }
  });
}