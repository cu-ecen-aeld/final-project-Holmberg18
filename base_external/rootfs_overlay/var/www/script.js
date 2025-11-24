let errorCount = 0;
const maxErrors = 3;

function updateTemperature() {
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
    updateTemperature();

    // update temp every 2 seconds
    setInterval(updateTemperature, 2000);
})