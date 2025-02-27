// Create events for the sensor readings
if (!!window.EventSource) {
  let eventSource = new EventSource('/events');

  eventSource.addEventListener('open', function (e) {
  }, false);

  eventSource.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
    }
  }, false);

  eventSource.addEventListener('gyroData', function (e) {
    const gyroData = JSON.parse(e.data);
    updateGyroChart(gyroData);
    document.getElementById("gyroX").innerHTML = gyroData.gyroX;
    document.getElementById("gyroY").innerHTML = gyroData.gyroY;
    document.getElementById("gyroZ").innerHTML = gyroData.gyroZ;
  }, false);

  eventSource.addEventListener('temperatureData', function (e) {
    updateTempChart(parseFloat(e.data));
    document.getElementById("bmpTemperature").innerHTML = e.data;
  }, false);

  eventSource.addEventListener('accelerometerData', function (e) {
    const accData = JSON.parse(e.data);
    updateAccChart(accData);
    document.getElementById("accX").innerHTML = " " + accData.accX;
    document.getElementById("accY").innerHTML = " " + accData.accY;
    document.getElementById("accZ").innerHTML = " " + accData.accZ;
  }, false);

  eventSource.addEventListener('bmpData', function (e) {
    const bmpData = JSON.parse(e.data);
    updatePressureChart(bmpData.bmpPressure);
    document.getElementById("bmp").innerHTML = bmpData.bmpPressure;
  }, false);

  eventSource.addEventListener('user_status', updateUserStatus, false);
} else {
  console.log("Unable to connect to event source!");
}

const updateUserStatus = (e) => {
  console.log("Changing image: ", e.data);
  switch (e.data) {
    case "WALKING":
      document.getElementById("user_status").innerHTML = "The user is WALKING";
      document.getElementById("activity_image").src = "images\\personWalking.png";
      break;
    case "STAIRS":
      document.getElementById("user_status").innerHTML = "The user is TAKING THE STAIRS";
      document.getElementById("activity_image").src = "images\\personOnStairs.png";
      break;
    case "ELEVATOR":
      document.getElementById("user_status").innerHTML = "The user is IN AN ELEVATOR";
      document.getElementById("activity_image").src = "images\\personInElevator.png";
      break;
    default:
      document.getElementById("user_status").innerHTML = "The user is AT REST";
      document.getElementById("activity_image").src = "images\\personAtRest.png";
      break;
  }
}

let lastDate = new Date().getTime();

let dataAccX = [];
let dataAccY = [];
let dataAccZ = [];
let dataGyroX = [];
let dataGyroY = [];
let dataGyroZ = [];

function createAccelerometerChart() {
  let chartOpt = {
    series: [{
      name: 'Acc X',
      data: dataAccX.slice()
    }, {
      name: 'Acc Y',
      data: dataAccY.slice()
    }, {
      name: 'Acc Z',
      data: dataAccZ.slice()
    }],
    chart: {
      foreColor: "#373D3F",
      id: 'realtime',
      height: 350,
      type: 'line',
      animations: {
        enabled: true,
        easing: 'linear',
        dynamicAnimation: {
          speed: 500
        }
      },
      toolbar: {
        show: false
      },
      zoom: {
        enabled: false
      },
      events: {
      },
      tooltip: {
        enabled: true,
        enabledOnSeries: undefined,
        shared: true,
        followCursor: true,
        intersect: false,
        fillSeriesColor: false,
      }
    },
    dataLabels: {
      enabled: false
    },
    stroke: {
      curve: 'smooth'
    },
    markers: {
      size: 0
    },
    xaxis: {
      type: 'datetime',
      range: 100000
    },
    yaxis: {
      max: 100,
      min: -100
    },
    legend: {
      show: true,
      position: 'top'
    }
  };

  let chart = new ApexCharts(document.querySelector("#accChart"), chartOpt);
  chart.render();
  return chart;
}

let accChart = createAccelerometerChart();

const updateAccChart = (obj) => {
  lastDate += 1000;
  console.log(lastDate);
  let newDataAccX = {
    x: lastDate,
    y: parseFloat(obj.accX)
  };
  let newDataAccY = {
    x: lastDate,
    y: parseFloat(obj.accY)
  };
  let newDataAccZ = {
    x: lastDate,
    y: parseFloat(obj.accZ)
  };

  dataAccX.push(newDataAccX);
  dataAccY.push(newDataAccY);
  dataAccZ.push(newDataAccZ);

  if (dataAccX.length > 100) {
    dataAccX.shift();
  }
  if (dataAccY.length > 100) {
    dataAccY.shift();
  }
  if (dataAccZ.length > 100) {
    dataAccZ.shift();
  }

  let maxAccX = Math.max(...dataAccX.map(item => item.y));
  let maxAccY = Math.max(...dataAccY.map(item => item.y));
  let maxAccZ = Math.max(...dataAccZ.map(item => item.y));
  let maxYValue = Math.max(maxAccX, maxAccY, maxAccZ);

  accChart.updateOptions({
    yaxis: {
      max: maxYValue,
      min: -maxYValue
    }
  });

  // Update each series individually
  accChart.updateSeries([{
    name: 'accX',
    data: dataAccX
  }, {
    name: 'accY',
    data: dataAccY
  }, {
    name: 'accZ',
    data: dataAccZ
  }]);
}

const createGyroChart = () => {
  let chartOpt = {
    series: [{
      name: 'Gyro X',
      data: dataGyroX.slice()
    }, {
      name: 'Gyro Y',
      data: dataGyroY.slice()
    }, {
      name: 'Gyro Z',
      data: dataGyroZ.slice()
    }],
    chart: {
      foreColor: "#373D3F",
      id: 'realtime',
      height: 350,
      type: 'line',
      animations: {
        enabled: true,
        easing: 'linear',
        dynamicAnimation: {
          speed: 500
        }
      },
      toolbar: {
        show: false
      },
      zoom: {
        enabled: false
      },
      events: {
      },
      tooltip: {
        enabled: true,
        enabledOnSeries: undefined,
        shared: true,
        followCursor: true,
        intersect: false,
        fillSeriesColor: false,
      }
    },
    dataLabels: {
      enabled: false
    },
    stroke: {
      curve: 'smooth'
    },
    markers: {
      size: 0
    },
    xaxis: {
      type: 'datetime',
      range: 100000
    },
    yaxis: {
      max: 100,
      min: -100
    },
    legend: {
      show: true,
      position: 'top'
    }
  };

  let gyroChart = new ApexCharts(document.querySelector("#gyroChart"), chartOpt);
  gyroChart.render();
  return gyroChart;
}

let gyroChart = createGyroChart();


const updateGyroChart = (obj) => {
  lastDate += 1000;
  console.log(lastDate);
  let newDataGyroX = {
    x: lastDate,
    y: parseFloat(obj.gyroX)
  };
  let newDataGyroY = {
    x: lastDate,
    y: parseFloat(obj.gyroY)
  };
  let newDataGyroZ = {
    x: lastDate,
    y: parseFloat(obj.gyroZ)
  };

  dataGyroX.push(newDataGyroX);
  dataGyroY.push(newDataGyroY);
  dataGyroZ.push(newDataGyroZ);

  if (dataGyroX.length > 100) {
    dataGyroX.shift();
  }
  if (dataGyroY.length > 100) {
    dataGyroY.shift();
  }
  if (dataGyroZ.length > 100) {
    dataGyroZ.shift();
  }

  let maxAccX = Math.max(...dataGyroX.map(item => item.y));
  let maxAccY = Math.max(...dataGyroY.map(item => item.y));
  let maxAccZ = Math.max(...dataGyroZ.map(item => item.y));
  let maxYValue = Math.max(maxAccX, maxAccY, maxAccZ);

  gyroChart.updateOptions({
    yaxis: {
      max: maxYValue,
      min: -maxYValue
    }
  });

  gyroChart.updateSeries([{
    name: 'gyroX',
    data: dataGyroX
  }, {
    name: 'gyroY',
    data: dataGyroY
  }, {
    name: 'gyroZ',
    data: dataGyroZ
  }]);
}
let tempData = [];


const createTemperatureChart = () => {
  let chartOpt = {
    series: [0],
    chart: {
      foreColor: "#373D3F",
      height: 350,
      type: 'radialBar',
      animations: {
        enabled: true,
        easing: 'linear',
        dynamicAnimation: {
          speed: 1000
        }
      },
      toolbar: {
        show: false
      },
    },
    plotOptions: {
      radialBar: {
        startAngle: 0,
        endAngle: 360,
        hollow: {
          margin: 0,
          size: '70%',
        },
        dataLabels: {
          showOn: 'always',
          name: {
            show: true,
            fontSize: '22px'
          },
          value: {
            show: true,
            fontSize: '16px',
            formatter: function (val) {
              let tempVal = (val / 100) * (50 - 10) + 10;
              return Math.round(tempVal) + " °C";
            }
          }
        }
      }
    },
    stroke: {
      lineCap: 'round'
    },
    labels: ['Temperature'],
    colors: ['#554298']
  };

  let tempChart = new ApexCharts(document.querySelector("#tempChart"), chartOpt);
  tempChart.render();
  return tempChart;
}

let pressureData = [];

const createPressureChart = () => {
  let chartOpt = {
    series: [0],
    chart: {
      foreColor: "#373D3F",
      height: 350,
      type: 'radialBar',
      animations: {
        enabled: true,
        easing: 'linear',
        dynamicAnimation: {
          speed: 1000
        }
      },
      toolbar: {
        show: false
      }
    },
    plotOptions: {
      radialBar: {
        startAngle: -90,
        endAngle: 90,
        hollow: {
          margin: 0,
          size: '65%'
        },
        track: {
          startAngle: -90,
          endAngle: 90,
        },
        dataLabels: {
          showOn: 'always',
          name: {
            show: true,
            fontSize: '22px'
          },
          value: {
            show: true,
            fontSize: '16px',
            formatter: function (v) {
              let tempVal = (v / 100) * (1500 - 500) + 500;
              return Math.round(tempVal) + " Pa";
            }
          }
        }
      }
    },
    stroke: {
      lineCap: 'round'
    },
    labels: ['Pressure'],
    colors: ['#ff4560']
  };

  let pressureChart = new ApexCharts(document.querySelector("#pressureChart"), chartOpt);
  pressureChart.render();
  return pressureChart;
}

let pressureChart = createPressureChart();

const updatePressureChart = (pressureData) => {
  const pressureInfo = (pressureData - 500) / (1500 - 500) * 100;
  pressureChart.updateSeries([pressureInfo]);
}

let tempChart = createTemperatureChart();

const updateTempChart = (tempData) => {
  let tempInfo = (tempData - 10) / (50 - 10) * 100;
  tempChart.updateSeries([tempInfo]);
}