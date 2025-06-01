import { startWebSocket } from "./ws-handler.js";


const drawLineChart = (id, labels, data, label) => {
    new Chart(document.getElementById(id), {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          label: label,
          data: data,
          borderColor: 'rgba(54, 162, 235, 1)',
          backgroundColor: 'rgba(54, 162, 235, 0.2)',
          fill: true,
          tension: 0.3
        }]
      },
      options: {
        scales: {
          y: {
            beginAtZero: true
          }
        }
      }
    });
  };

const doubleLineChart = (id, labels, dataSets, labelsForData) => {
  const colors = ['rgba(54, 162, 235, 1)', 'purple'];
  const bgColors = ['rgba(54, 162, 235, 0.2)', 'rgba(70, 70, 70, 0.3)'];

  const datasets = dataSets.map((data, index) => ({
    label: labelsForData[index],
    data: data,
    borderColor: colors[index],
    backgroundColor: bgColors[index],
    fill: true,
    tension: 0.3
  }));

  new Chart(document.getElementById(id), {
    type: 'line',
    data: {
      labels: labels,
      datasets: datasets
    },
    options: {
      responsive: true,
      scales: {
        y: {
          beginAtZero: true
        }
      }
    }
  });
}

  const drawDoughnutChart = (id, value, fillcolor) => {
    const centerTextPlugin = {
      id: 'centerText',
      beforeDraw: (chart) => {
        const {width, height, ctx} = chart;
        ctx.restore();
        const fontSize = (height / 7).toFixed(2);
        ctx.font = `${fontSize}px sans-serif`;
        ctx.textBaseline = 'top';
        ctx.fillStyle = '#333';
        const text = `${value}%`;
        const textX = Math.round((width - ctx.measureText(text).width) / 2);
        const textY = height / 2;
        ctx.fillText(text, textX, textY);
        ctx.save();
      }
    };
  
    new Chart(document.getElementById(id), {
      type: 'doughnut',
      data: {
        datasets: [{
          data: [value, 100 - value],
          backgroundColor: [fillcolor, '#eeeeee'],
          borderWidth: 0,
        }]
      },
      options: {
        cutout: '78%',
        rotation: -90,
        circumference: 180,
        animation: false,
        plugins: {
          tooltip: { enabled: false },
          legend: { display: false }
        }
      },
      plugins: [centerTextPlugin]
    });
  };
  


let charging = false;
function getDoughnutColor(charging) {
    return charging ? '#2ecc71' : '#b0b0b0';
  }


// Real-time updates via WebSocket
startWebSocket(
  (soc, soh) => {
    const socElem = document.getElementById("soc-percent");
    const sohElem = document.getElementById("sohChartBar");
    if (socElem) socElem.textContent = `${soc}%`;
    if (sohElem) sohElem.value = soh;
    drawDoughnutChart("socChartDoughnut", soc, getDoughnutColor(charging));
  },
  (batteryTemp, externalTemp) => {
    doubleLineChart(
      "tempChart",
      ["지금"],
      [[batteryTemp], [externalTemp]],
      ["배터리 온도", "외부 온도"]
    );
  },
  (voltage) => {
    drawLineChart(
      "voltageChart",
      ["지금"],
      [voltage],
      "전압"
    );
  }
);






  // drawDoughnutChart('socChartDoughnut', 60, getDoughnutColor(charging));
  // doubleLineChart(
  //   'tempChart',
  //   ['00:00', '00:10', '00:20'],
  //   [
  //     [27, 25, 22],
  //     [25, 26, 26.5]
  //   ],
  //   ['배터리 온도', '외부 온도']
  // );
  // drawLineChart('voltageChart', ['00','04','08','12','16','20','24'], [380, 382, 385, 384, 383, 382, 384], '전압');