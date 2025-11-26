let errorCount = 0;
const maxErrors = 3;

function updateTempDisplay(data){
    // Update temp display
    const tempDisplay = document.getElementById('temperatureDisplay');
    tempDisplay.textContent = `${data.temperature}°C`;


    // Temp color codes
    tempDisplay.className = 'temperature-display';
    if(data.temperature < 50){
        tempDisplay.classList.add('temp-low');
    } else if (data.temperature <= 70){
        tempDisplay.classList.add('temp-medium');
    } else {
        tempDisplay.classList.add('temp-high');
    }
}

function updateFanSpeedDisplay(data){
    // Update fan speed display
    const fanSpeed = data.fan_speed;
    document.getElementById('fan-speed-text').textContent = `Fan Speed: ${fanSpeed}%`;
    document.getElementById('fan-visual').style.width = `${fanSpeed}%`;
    
    const fanStateElement = document.getElementById('fan-state');
    let statusText = '';
    
    fanStateElement.classList.remove('fan-cool', 'fan-warm', 'fan-hot');
    
    // Add fan speed status based on fan speed
    if(fanSpeed === 0){
        statusText = 'Status: System Cool';
        fanStateElement.classList.add('fan-cool');
    } else if(fanSpeed <= 50){
        statusText = 'Status: System Warm';
        fanStateElement.classList.add('fan-warm');
    } else {
        statusText = 'Status: System Hot - Cooling Active';
        fanStateElement.classList.add('fan-hot');
    }

    fanStateElement.textContent = statusText;
}

function updateResourceDisplay(data){
    // CPU usage readings
    const cpuUsage = data.cpu_usage !== undefined ? data.cpu_usage : -1;
    if(cpuUsage >= 0){
        document.getElementById('cpu-usage').textContent = cpuUsage.toFixed(1);
        document.getElementById('cpu-bar').style.width = `${cpuUsage}%`;
    } else {
        document.getElementById('cpu-usage').textContent = 'N/A';
        document.getElementById('cpu-bar').style.width = '0%';
    }

    // Memory usage readings
    const memUsage = data.memory_usage !== undefined ? data.memory_usage : -1;
    if(memUsage >= 0){
        const memUsedMB = (data.memory_used / 1024).toFixed(1);
        const memTotalMB = (data.memory_total / 1024).toFixed(1);

        document.getElementById('memory-usage').textContent = memUsage.toFixed(1);
        document.getElementById('memory-bar').style.width = `${memUsage}%`;
        document.getElementById('memory-detail').textContent =
            `${memUsedMB} MB / ${memTotalMB} MB`;
    } else {
        document.getElementById('memory-usage').textContent = 'N/A';
        document.getElementById('memory-bar').style.width = '0%';
        document.getElementById('memory-detail').textContent = 'Memory data unavailable';
    }

    // Load averages
    if(data.load_1min !== undefined){
        document.getElementById('load-1min').textContent = data.load_1min.toFixed(2);
        document.getElementById('load-5min').textContent = data.load_5min.toFixed(2);
        document.getElementById('load-15min').textContent = data.load_15min.toFixed(2);
    } else {
        document.getElementById('load-1min').textContent = "--";
        document.getElementById('load-5min').textContent = '--';
        document.getElementById('load-15min').textContent = '--';
    }
}

function updateDisplay() {

    document.getElementById('lastUpdate').textContent = 'Updating...';
    fetch("system_stats.json")
        .then(response => {
            if(!response.ok){
                throw new Error("Network response invalid");
            }
            return response.json();
        })
        .then(data => {
            // If success, hide the error message
            document.getElementById('errorMessage').style.display = 'none';
            errorCount = 0;

            updateTempDisplay(data);
            updateFanSpeedDisplay(data);
            updateResourceDisplay(data);


            // Update the timestamp
            document.getElementById('lastUpdate').textContent =
                `Last updated: ${data.timestamp || new Date().toLocaleString()}`;
        })
        .catch(error => {
            console.error('Error fetching temperature:', error);
            errorCount++;

            if(errorCount >= maxErrors){
                document.getElementById('errorMessage').style.display = 'block';
            }
        });
}

document.addEventListener('DOMContentLoaded', function() {
    updateDisplay();

    // update temp every 2 seconds
    setInterval(updateDisplay, 2000);
})