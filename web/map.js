/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

const overlay = document.getElementById("overlay");
const altSlider = document.getElementById("alt-slider");
const notify = document.getElementById("notify");
const notifyText = document.getElementById("notify-text");
const altIn = document.getElementById("alt-input");
const altVal = document.getElementById("alt-value");
const altBtn = document.getElementById("alt-btn");

// Map initial zoom and location is here
const map = L.map("map").setView([20, 0], 2);
const maxZoom = 19;


function map_init() {
    // Add layers
    const googleHybrid = L.tileLayer("https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}", {
        maxZoom: maxZoom,
        attribution: "Imagery &copy; CNES / Airbus, Landsat / Copernicus, Maxar Technologies, Sanborn, TerraMetrics, U.S. Geological Survey, USDA/FPAC/GEO, Map data &copy; 2023 Google"
    });
    const googleSatellite = L.tileLayer("https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}", {
        maxZoom: maxZoom,
        attribution: "Imagery &copy; CNES / Airbus, Landsat / Copernicus, Maxar Technologies, Sanborn, TerraMetrics, U.S. Geological Survey, USDA/FPAC/GEO, Map data &copy; 2023"
    });
    const googleMap = L.tileLayer("https://mt1.google.com/vt/lyrs=m&x={x}&y={y}&z={z}", {
        maxZoom: maxZoom,
        attribution: "Map data &copy; 2023 Google"
    });
    const openMap = L.tileLayer("https://tile.openstreetmap.org/{z}/{x}/{y}.png", {
        maxZoom: maxZoom,
        attribution: "Map data &copy; <a href=\"http://www.openstreetmap.org/copyright\">OpenStreetMap</a> and contributors"
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
    map_setAlt(function() {
        var marker;
        if (lat == null && lng == null) {
            // Coordinates given from map
            lat = event.latlng.lat;
            lng = event.latlng.lng;
        } else {
            // Coordinates given manually
            changeButton(manaddButton, "#4CAF50", "Added!");
            manaddTimeout = setTimeout(() => {
                changeButton(manaddButton, "#A041DB", "Add Waypoint");
            }, btnTimeout);
        }
        // Check to make sure the coordinates are valid before accepting them
        if (lat < -85.05112878 || lat > 85.05112878 || lng < -180 || lng > 180) {
            notifyText.innerHTML = "Invalid coordinates. Please try again.";
            overlay.style.display = "block";
            notify.style.display = "block";
            setTimeout(() => {
                overlay.style.display = "none";
                notify.style.display = "none";
            }, 3500);
            changeButton(manaddButton, "#A041DB", "Add Waypoint");
            return;
        }
        fplan.waypoints.push({lat: lat, lng: lng, alt: altIn.value});
        // Create a visual marker and add it to the map
        marker = L.marker([lat, lng]).addTo(map);
        markers.push(marker);
        
        // Draw lines between the markers
        if (markers.length > 1) {
            polylines.push(L.polyline(markers.map(marker => marker.getLatLng()), {color:"#D21404"}).addTo(map));
        }
        // Add a click listener to the marker for the removal function so it can be removed later
        marker.addEventListener("click", map_removeWpt.bind(marker));
        
        // Enable the unload prompt because a new waypoint (presumably unsaved) has been generated
        promptBeforeUnload = true;
        // If the flightplan has been generated and another waypoint is added, make the regen button visible
        if (fplanGenerated) {
            genButtonCopyState = false;
            changeButton(genButton, "#A6710C", "Generate Flightplan");
        }
    });
}

function map_removeWpt() {
    // Remove the waypoint from the flight plan
    fplan.waypoints.splice(markers.indexOf(this), 1);
    map.removeLayer(this);
    markers.splice(markers.indexOf(this), 1);

    // Remove all existing polylines from the map
    polylines.forEach(polyline => map.removeLayer(polyline));
    polylines = [];
    // Redraw the lines between the remaining markers
    if (markers.length > 1) {
        polylines.push(L.polyline(markers.map(marker => marker.getLatLng()), {color:"#D21404"}).addTo(map));
    }
    if (fplanGenerated) {
        genButtonCopyState = false;
        changeButton(genButton, "#A6710C", "Generate Flightplan");
    }
    // Check to see if we have removed all waypoints, if so update the reload dialog flag
    if (fplan.waypoints.length == 0) {
        promptBeforeUnload = false;
    }
}

function map_setAlt(callback) {
    overlay.style.display = "block";
    altSlider.style.display = "block";

    altBtn.addEventListener("click", handleAltButtonClick);
    function handleAltButtonClick() {
        altSet();
    }

    overlay.addEventListener("click", handleOverlayClick);
    function handleOverlayClick(event) {
        if (event.target === overlay) {
            altSet();
        }
    }

    document.addEventListener("keydown", handleKeyDown);
    function handleKeyDown(event) {
        if (event.key === "Enter") {
            altSet();
        } else if (event.key === "Escape") {
            overlay.style.display = "none";
            altSlider.style.display = "none";
            removeEventListeners();
            promptBeforeUnload = false;
        }
    }

    function altSet() {
        overlay.style.display = "none";
        altSlider.style.display = "none";
        callback();
        removeEventListeners();
    }

    function removeEventListeners() {
        altBtn.removeEventListener("click", handleAltButtonClick);
        overlay.removeEventListener("click", handleOverlayClick);
        document.removeEventListener("keydown", handleKeyDown);
    }
}


map_init();
map.addEventListener("click", map_addWpt);

altIn.addEventListener("input", function() {
    altVal.innerHTML = this.value;
});
