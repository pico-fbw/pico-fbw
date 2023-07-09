/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

/* Reference constants */

const altSlider = document.getElementById("alt-slider");
const notifyText = document.getElementById("notify-text");
const altIn = document.getElementById("alt-input");
const altVal = document.getElementById("alt-value");
const altBtn = document.getElementById("alt-btn");

// Map initial zoom and location is here
const map = L.map("map").setView([20, 0], 2);
const maxZoom = 19;

/* State variables */

var markers = [];
var polylines = [];

/* Global functions */

/**
 * Adds a waypoint to the map, either through means of clicking it or manually specifying coordinates.
 * @param {Event} event If adding a waypoint by click, pass the event here.
 * @param {String} lat If adding a waypoint by specifiying coordinates, pass latitude in here.
 * @param {String} lng If adding a waypoint by specifiying coordinates, pass longitude in here.
 */
function map_addWpt(event, lat, lng) {
    map_setAlt(function() {
        var marker;
        if (lat == null && lng == null) {
            // Coordinates given from map
            lat = event.latlng.lat;
            lng = event.latlng.lng;
        } else {
            // Coordinates given manually
            btn_change(manaddButton, state_adding);
            manaddTimeout = setTimeout(() => {
                btn_change(manaddButton, state_addWpt);
            }, btnTimeout);
        }
        // Check to make sure the coordinates are valid before accepting them
        if (lat < -85.05112878 || lat > 85.05112878 || lng < -180 || lng > 180) {
            window_displayNotification("Invalid coordinates. Please try again.", true);
            btn_change(manaddButton, state_addWpt);
            return;
        }
        fplan.waypoints.push({lat: lat, lng: lng, alt: altIn.value});
        // Create a visual marker and add it to the map
        marker = L.marker([lat, lng]).addTo(map);
        markers.push(marker);
        
        // Draw lines between the markers
        if (markers.length > 1) {
            polylines.push(
                L.polyline(markers.map(marker => marker.getLatLng()), {color: polyLineColor}).addTo(map)
            );
        }
        // Add a click listener to the marker for the removal function so it can be removed later
        marker.addEventListener("click", function() { map_removeWpt(markers.indexOf(marker)); });
        // Enable dragging of the marker and bind it to our drag function
        marker.dragging.enable();
        marker.addEventListener("dragend", map_wptDragged.bind(marker));
        
        // Enable the unload prompt because a new waypoint (presumably unsaved) has been generated
        window_setPromptBeforeUnload(true);
        // If the flightplan has been generated and another waypoint is added, make the regen button visible
        if (fplanGenerated) {
            genButtonState = btnState_generate;
            btn_change(genButton, state_generate);
        }
    });
}

/**
 * Removes a waypoint from the flightplan and map.
 * @param {Number} index The index of the waypoint (in the flightplan array) to remove.
 */
function map_removeWpt(index) {
    // Remove the waypoint from the flight plan
    fplan.waypoints.splice(index, 1);
    // Remove the marker visually from the map
    map.removeLayer(markers[index]);
    markers.splice(index, 1);
    // Remove all existing polylines from the map
    polylines.forEach(polyline => map.removeLayer(polyline));
    polylines = [];
    // Redraw the lines between the remaining markers
    if (markers.length > 1) {
        polylines.push(
            L.polyline(markers.map(marker => marker.getLatLng()), {color: polyLineColor}).addTo(map)
        );
    }
    if (fplanGenerated) {
        genButtonState = btnState_generate;
        btn_change(genButton, state_generate);
    }
    // Check to see if we have removed all waypoints, if so update the reload dialog flag
    if (fplan.waypoints.length == 0) {
        window_setPromptBeforeUnload(false);
    }
}

/* Local functions */

/**
 * Initalizes the Wi-Fly map.
 */
function map_init() {
    // Add layers
    const googleHybrid = L.tileLayer("https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}", {
        maxZoom: maxZoom,
        attribution: "Imagery &copy; Google Satellite Imagery Sources, Map data &copy; 2023 Google"
    });
    const googleSatellite = L.tileLayer("https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}", {
        maxZoom: maxZoom,
        attribution: "Imagery &copy; Google Satellite Imagery Sources, Map data &copy; 2023"
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

/**
 * Callback for if a waypoint is dragged on the map.
 */
function map_wptDragged() {
    // Update the flightplan with the waypoint's new coords
    var index = markers.indexOf(this);
    if (index != -1) {
        fplan.waypoints[index].lat = this.getLatLng().lat;
        fplan.waypoints[index].lng = this.getLatLng().lng;
    }
    // Same things happen above in removeWpt; redraw lines, update button
    polylines.forEach(polyline => map.removeLayer(polyline));
    polylines = [];
    if (markers.length > 1) {
        polylines.push(
            L.polyline(markers.map(marker => marker.getLatLng()), {color: polyLineColor}).addTo(map)
        );
    }
    if (fplanGenerated) {
        genButtonState = btnState_generate;
        btn_change(genButton, state_generate);
    }
    window_setPromptBeforeUnload(true);
}

/**
 * Initalizes the altitude selection dialog, with a callback functionality.
 * The callback function will be called after the user completes the dialog.
 * @param {Function} callback The callback function.
 */
function map_setAlt(callback) {
    // Show the alt slider and disable scrolling
    overlay.style.display = "block";
    altSlider.style.display = "block";
    document.body.style.overflow = "hidden";
    // Add event listeners for ok button and outside slider
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
    // Add event listeners for enter and escape keys
    document.addEventListener("keydown", handleKeyDown);
    function handleKeyDown(event) {
        if (event.key === "Enter") {
            altSet();
        } else if (event.key === "Escape") {
            overlay.style.display = "none";
            altSlider.style.display = "none";
            removeEventListeners();
            window_setPromptBeforeUnload(false);
            document.body.style.overflow = "auto"; // Enable scrolling
        }
    }
    // Functions to set and cancel altitude
    function altSet() {
        overlay.style.display = "none";
        altSlider.style.display = "none";
        callback();
        removeEventListeners();
        document.body.style.overflow = "auto";
    }

    function removeEventListeners() {
        altBtn.removeEventListener("click", handleAltButtonClick);
        overlay.removeEventListener("click", handleOverlayClick);
        document.removeEventListener("keydown", handleKeyDown);
    }
}

/* Runtime code*/

// Initalize map and add its event listeners
map_init();
map.addEventListener("click", map_addWpt);

altIn.addEventListener("input", function() {
    altVal.innerHTML = this.value;
});
