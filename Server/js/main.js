var temp = [], hum = [], time = [], state = [], xlabel = [];
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
        console.log(snap[i][n])
        const tmp1 = snap[i][n].split(' ');
        const tmp2 = snap[i][n].split('');
        xlabel.push(tmp1[0]+", "+tmp1[1]+" "+tmp1[2]+" "+tmp1[4]);
        time.push(tmp2[11]+tmp2[12]+tmp2[13]+tmp2[14]+tmp2[15]);
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
    status='Operating';
  }
  else if (state[9]==0){
    status='Stopped';
  }
  else{
    status='Halted';
  }

  document.getElementById("status").innerHTML = "Status: " + status;

  drawTemperature(time, xlabel[0], temp);
  drawHumidity(time, xlabel[0], hum);
  drawGraphStatus(time, xlabel[0], state);
});

      

function drawTemperature(label, label_title, graph) {
  var ctx = document.getElementById("Temperature").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: label,
      datasets: [{
        label: "Temperature",
        //labelString: "ºC",
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
      aspectRatio: 1.8,
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
            beginAtZero: false,
            stepSize: .5,
            Max: 100,
            Min: 0,
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
        //labelString: "%",
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
      aspectRatio: 1.8,
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
            beginAtZero: false,
            stepSize: .5,
            Max: 100,
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
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      aspectRatio: 1.8,
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
          },
          scaleLabel: {
            display: true,
            labelString: 'Value',
            fontSize: 18,
            fontColor: '#BBC1CB',
            fontFamily: 'Helvetica Neue',
          }
        }],
      }
    }
  });
}