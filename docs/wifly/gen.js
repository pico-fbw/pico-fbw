/* Declare constants and vars*/

var fplan = {
    // Versioncode is here, change when there is a new release
    version: "0.1a",
    waypoints: []
};
// Maximum length of the generated flightplan JSON (URL-encoded)
const maxFplanLen = 8760;

const field = document.getElementById("fplan");
const genButton = document.getElementById("generate");
const regenButton = document.getElementById("regenerate");

// Map initial zoom and location is here
const map = L.map("map").setView([20, 0], 2);
const maxZoom = 19;

/* Function definitions */

/**
 * Generates the current flightplan into JSON and puts it into the fplan field.
 * If the flightplan is too long nothing will be generated.
 * 
 * @returns true if the flightplan was generated, false if it was not.
 */
function wifly_genFplan() {
    // Convert the object to JSON string
    var fplanJSON = JSON.stringify(fplan, null, 0);
    // Encode it into a URL temporarily so we can check its length (this is how the Pico will recieve it)
    if (encodeURI(fplanJSON).length > maxFplanLen) {
        return false;
    } else {
        field.value = JSON.stringify(fplan, null, 0);
        return true;
    }
}

var genButtonCopyState = false;
var genTimeout;
/**
 * Callback function for the generate flightplan button.
 */
function genButtonCallback() {
    if (fplan.waypoints.length < 2) {
        genButton.style.backgroundColor = "#D21404";
        genButton.innerHTML = `Please select two or more waypoints (currently ${fplan.waypoints.length})!`;
        genTimeout = setTimeout(() => {
            genButton.style.backgroundColor = "#5A5A5A";
            genButton.innerHTML = "Generate Flightplan";
        }, "4000");
        return;
    }
    if (!genButtonCopyState) {
        // Generate the flightplan and handle any errors
        if (!wifly_genFplan()) {
            genButton.style.backgroundColor = "#D21404";
            genButton.innerHTML = "Flightplan too long!";
            genTimeout = setTimeout(() => {
                genButton.style.backgroundColor = "#5A5A5A";
                genButton.innerHTML = "Generate Flightplan";
            }, "4000");
            return;
        }
        // Update color and text of button to indicate successful generation
        clearTimeout(genTimeout); // This is so the button doesn't change back to generate if the user creates waypoints quickly
        genButton.style.backgroundColor = "#008CBA";
        genButton.innerHTML = "Copy to clipboard";
        regenButton.style.visibility = "visible";
        genButtonCopyState = true;
    } else {
        clearTimeout(genTimeout);
        navigator.clipboard.writeText(field.value).then(() => {
            genButton.style.backgroundColor = "#4CAF50";
            genButton.innerHTML = "Copied!";
            genTimeout = setTimeout(() => {
                genButton.style.backgroundColor = "#008CBA";
                genButton.innerHTML = "Copy to clipboard";
            }, "4000");
        });
    }
}
var regenTimeout;
function regenButtonCallback() {
    if (!wifly_genFplan()) {
        regenButton.style.backgroundColor = "#D21404";
        regenButton.innerHTML = "Flightplan too long!";
        regenTimeout = setTimeout(() => {
            regenButton.style.backgroundColor = "#E49B0F";
            regenButton.innerHTML = "Re-generate";
        }, "4000");
        return;
    }
    clearTimeout(regenTimeout);
    regenButton.style.backgroundColor = "#4CAF50";
    regenButton.innerHTML = "Re-generated!";
    regenTimeout = setTimeout(() => {
        regenButton.style.backgroundColor = "#E49B0F";
        regenButton.innerHTML = "Re-generate";
    }, "4000");
}

function map_init() {
    // Add layers
    const googleHybrid = L.tileLayer('https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', {
        maxZoom: maxZoom,
        attribution: 'Imagery &copy; CNES / Airbus, Landsat / Copernicus, Maxar Technologies, Sanborn, TerraMetrics, U.S. Geological Survey, USDA/FPAC/GEO, Map data &copy; 2023 Google'
    });
    const googleSatellite = L.tileLayer('https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}', {
        maxZoom: maxZoom,
        attribution: 'Imagery &copy; CNES / Airbus, Landsat / Copernicus, Maxar Technologies, Sanborn, TerraMetrics, U.S. Geological Survey, USDA/FPAC/GEO, Map data &copy; 2023'
    });
    const googleMap = L.tileLayer('https://mt1.google.com/vt/lyrs=m&x={x}&y={y}&z={z}', {
        maxZoom: maxZoom,
        attribution: 'Map data &copy; 2023 Google'
    });
    const openMap = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: maxZoom,
        attribution: 'Map data &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> and contributors'
    })
    // Create layer control
    const mapLayers = {
        "Hybrid" : googleHybrid,
        "Satellite only" : googleSatellite,
        "Map only" : googleMap,
        "Map only (OpenStreetMap)" : openMap
    };
    // Add the layer control to the map
    L.control.layers(mapLayers).addTo(map);
    // Set default layer
    googleHybrid.addTo(map);
}

var markers = [];
function map_onClick(event) {
    // Get the lat/long coordinates of the click
    var coords = event.latlng;
    // Push these coordinates to the flightplan
    fplan.waypoints.push({lat: coords.lat, lng: coords.lng, alt: -1});

    // Create a visual marker and add it to the map
    var marker = L.marker(coords).addTo(map);
    // Add the marker to the marker array
    markers.push(marker);
    // Draw lines between the markers
    if (markers.length > 1) {
        var latlngs = markers.map(marker => marker.getLatLng());
        L.polyline(latlngs, {color : 'purple'}).addTo(map);
    }
}


/* Begin program execution */

// Attach event listeners to buttons
genButton.addEventListener("click", genButtonCallback);
regenButton.addEventListener("click", regenButtonCallback);

// Initialize the map and bind its click method
map_init();
map.on('click', map_onClick);
