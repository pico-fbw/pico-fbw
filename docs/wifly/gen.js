/* Declare constants and vars*/

var fplan = {
    version: "1.0",
    waypoints: []
};

const field = document.getElementById("fplan");
const genButton = document.getElementById("generate");

// Map initial zoom and location is here
const map = L.map("map").setView([20, 0], 2);

/* Function definitions */

/**
 * @returns a Wi-Fly flightplan as a string
 */
function wifly_genFplan() {
    // Convert the object to JSON string
    return JSON.stringify(fplan, null, 0);
}

var genButtonCopyState = false;
/**
 * Callback function for the generate flightplan button.
 */
function genButtonCallback() {
    if (fplan.waypoints.length < 2) {
        genButton.style.backgroundColor = "#D21404";
        genButton.innerHTML = "Please select two or more waypoints!";
        setTimeout(() => {
            genButton.style.backgroundColor = "#5A5A5A";
            genButton.innerHTML = "Generate Flightplan";
        }, "2000");
        return;
    }
    if (!genButtonCopyState) {
        // Update the output field with the generated flight plan JSON
        field.value = wifly_genFplan();
        // Update color and text of element to indicate successful generation
        genButton.style.backgroundColor = "#008CBA";
        genButton.innerHTML = "Copy to clipboard";
        genButtonCopyState = true;
    } else {
        const cb = navigator.clipboard;
        cb.writeText(field.value).then(() => {
            genButton.style.backgroundColor = "#4CAF50";
            genButton.innerHTML = "Copied!";
        });
    }
}

function map_init() {
    // Set correct tiles
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
    }).addTo(map);
}

function map_onClick(event) {
    var coords = event.latlng;
    fplan.waypoints.push({lat: coords.lat, long: coords.lng, alt: -1})
    // TODO: add clicked coords to an array and also display them on map, possibly connect them with a line?
}


/* Begin program execution */

// Attach event listener to the generate button
genButton.addEventListener("click", genButtonCallback);

// Initialize the map and bind its click method
map_init();
map.on('click', map_onClick);
