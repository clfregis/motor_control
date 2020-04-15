var temp = [], hum = [], time = [];


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
var database = firebase.database().ref("motor_1");

database.on('value', function (snapshot) {
  for (let i in snapshot.val().Temperature) {
    temp.push(snapshot.val().Temperature[i]);
  }
  for (let i in snapshot.val().Humidity) {
    hum.push(snapshot.val().Humidity[i]);
  }
  for (let i in snapshot.val().Time) {
    time.push(snapshot.val().Time[i]);
  }
  temp = temp.slice(temp.length - 20, temp.length);
  hum = hum.slice(hum.length - 20, hum.length);
  time = time.slice(time.lenght - 20, time.lenght);
  drawGraph(time, temp, hum);
});

function drawGraph(label, graph1, graph2) {
  var ctx = document.getElementById("myChart").getContext('2d');
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
        text: 'Weather Station - Motor 1'
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