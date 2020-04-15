// const CHART = document.getElementById('myChart');
// console.log(CHART);

// var temperature = [];

// var firebaseConfig = {
//     apiKey: "AIzaSyA-fPrOStfPGq-Ze_wW3mloB-Qo2AhgU8c",
//     authDomain: "esp32-66ba5.firebaseapp.com",
//     databaseURL: "https://esp32-66ba5.firebaseio.com",
//     projectId: "esp32-66ba5",
//     storageBucket: "esp32-66ba5.appspot.com",
//     messagingSenderId: "502036978763",
//     appId: "1:502036978763:web:5691be205391772c2a01ce",
//     measurementId: "G-85ZFMD2ZK8"
//   };
// // Initialize Firebase
// firebase.initializeApp(firebaseConfig);

// // Get a reference to the database service
// var database = firebase.database().ref("temp_test");


// database.on('value', function(snapshot){
// 	for(let i in snapshot.val().temp){
// 		temperature.push(snapshot.val().temperature[i]);
// 	}
// 	temperature = temperature.slice(temperature.length-20, temperature.length);
// });

// var labels = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19];


// let myChart = new Chart(CHART,{
//     type: 'line',
//  	data: {
//  		labels: labels,
// 	    datasets: [
// 		    {	
// 		    	label: "temperature [°C]",
// 		    	labelString : "°C",
// 		    	borderColor: 'rgb(255, 99, 132)',
// 		    	backgroundColor: 'rgb(255, 99, 132)',
// 		    	fill: false,
// 		    	data: temperature
// 		    }
// 	    ]
// 	},
// 	options: {
// 		responsive: true,
// 		maintainAspectRatio: false,
// 		title:{
// 			display: true,
// 			text:'Temperatura na sala',
// 		}
// 	},
// });


var bosta = [], oxygen = [];


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
var database = firebase.database().ref();

database.on('value', function (snapshot) {
  for (let i in snapshot.val().heartbeat) {
    bosta.push(snapshot.val().heartbeat[i]);
  }
  for (let i in snapshot.val().oxygen) {
    oxygen.push(snapshot.val().oxygen[i]);
  }
  bosta = bosta.slice(bosta.length - 20, bosta.length);
  oxygen = oxygen.slice(oxygen.length - 20, oxygen.length);
  drawGraph(bosta, oxygen);
});

function drawGraph(bosta, oxygen) {
  var labels = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19];
  var ctx = document.getElementById("myChart").getContext('2d');
  var myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: "heartbeat",
        labelString: "BPM",
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgb(255, 99, 132)',
        fill: false,
        data: bosta,
        yAxisID: "y-axis-heartbeat",
      },
      {
        label: "oxygen",
        labelString: "SpO2",

        borderColor: 'rgb(0, 99, 132)',
        backgroundColor: 'rgb(0, 99, 132)',
        fill: false,
        data: oxygen,
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
        text: 'Last 20'
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