/* Declare constants and vars*/

const overlay = document.getElementById("overlay");
const altIn = document.getElementById("alt-input");
const altVal = document.getElementById("alt-value");

// Map initial zoom and location is here
const map = L.map("map").setView([20, 0], 2);
const maxZoom = 19;


/* Function definitions */

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
var polylines = [];

function map_addWpt(event, lat, lng) {
    map_setAlt();
    var marker;
    if (lat == null && lng == null) {
        // Coordinates given from map
        // Push these coordinates to the flightplan
        fplan.waypoints.push({lat: event.latlng.lat, lng: event.latlng.lng, alt: -1});
        // Create a visual marker and add it to the map
        marker = L.marker(event.latlng).addTo(map);
    } else {
        // Coordinates given manually, adjust accordingly
        fplan.waypoints.push({lat: lat, lng: lng, alt: -1});
        marker = L.marker([lat, lng]).addTo(map);
    }
    // Add the marker to the marker array
    markers.push(marker);
    // Draw lines between the markers
    if (markers.length > 1) {
        polylines.push(L.polyline(markers.map(marker => marker.getLatLng()), {color:'#D21404'}).addTo(map));
    }
    // Add a click listener to the marker so it can be removed later
    marker.on('click', map_removeWpt.bind(marker));
    
    // If the flightplan has been generated and another waypoint is added, make the regen button visible
    if (fplanGenerated) {
        genButtonCopyState = false;
        changeButton(genButton, "#E49B0F", "Generate Flightplan");
    }
}

function map_removeWpt() {
    // Remove the waypoint from the flight plan
    fplan.waypoints.splice(markers.indexOf(this), 1);
    // Remove the marker from the map and the markers array
    map.removeLayer(this);
    markers.splice(markers.indexOf(this), 1);
    // Remove all existing polylines from the map
    polylines.forEach(polyline => map.removeLayer(polyline));
    polylines = [];
    // Redraw the lines between the remaining markers
    if (markers.length > 1) {
        polylines.push(L.polyline(markers.map(marker => marker.getLatLng()), {color:'#D21404'}).addTo(map));
    }
    if (fplanGenerated) {
        genButtonCopyState = false;
        changeButton(genButton, "#E49B0F", "Generate Flightplan");
    }
}

function map_setAlt() {
    // Show the overlay
    overlay.style.display = "block";
    
    // overlay.style.display = "none";
}


/* Begin program execution */

// Initialize the map and bind its click method
map_init();
map.on('click', map_addWpt);

// Add event listener to show the current altitude of slider
altIn.addEventListener("input", function() {
    altVal.innerHTML = this.value;
});
