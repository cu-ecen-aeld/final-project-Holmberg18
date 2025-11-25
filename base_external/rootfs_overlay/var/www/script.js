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

function updateDisplay() {
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


            // Update the timestamp
            document.getElementById('lastUpdate').textContent =
                `Last updated: ${data.timestamp}`;
        })
        .catch(error => {
            console.error('Error fetching temperature:', error);
            errorCount++;

            if(errorCount >= maxErrors){
                document.getElementById(`errorMessage`).style.display = 'block';
            }
        });
}

document.addEventListener('DOMContentLoaded', function() {
    updateDisplay();

    // update temp every 2 seconds
    setInterval(updateDisplay, 2000);
})