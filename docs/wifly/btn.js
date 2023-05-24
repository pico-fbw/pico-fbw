/* Declare constants and vars*/

// Time in milliseconds before a button reverts to its previous state
const btnTimeout = "3000";

const genButton = document.getElementById("generate");
const regenButton = document.getElementById("regenerate");
const manaddButton = document.getElementById("manadd_btn");
const manaddLat = document.getElementById("manadd_lat");
const manaddLng = document.getElementById("manadd_lng");


/* Function definitions */

var genButtonCopyState = false;
var genTimeout;
function genButtonCallback() {
    clearTimeout(genTimeout); // This is so the button doesn't change back to generate if the user creates waypoints quickly
    if (fplan.waypoints.length < 2) {
        genButton.style.backgroundColor = "#D21404";
        genButton.innerHTML = `Please select two or more waypoints (currently ${fplan.waypoints.length})!`;
        genTimeout = setTimeout(() => {
            genButton.style.backgroundColor = "#E49B0F";
            genButton.innerHTML = "Generate Flightplan";
        }, btnTimeout);
        return;
    }
    if (!genButtonCopyState) {
        // Generate the flightplan and handle any errors
        if (!wifly_genFplan()) {
            genButton.style.backgroundColor = "#D21404";
            genButton.innerHTML = "Flightplan too long!";
            genTimeout = setTimeout(() => {
                genButton.style.backgroundColor = "#E49B0F";
                genButton.innerHTML = "Generate Flightplan";
            }, btnTimeout);
            return;
        }
        // Update color and text of button to indicate successful generation
        field.style.display = "block";
        genButton.style.backgroundColor = "#008CBA";
        genButton.innerHTML = "Copy to clipboard";
        fplanGenerated = true;
        genButtonCopyState = true;
    } else {
        navigator.clipboard.writeText(field.value).then(() => {
            genButton.style.backgroundColor = "#4CAF50";
            genButton.innerHTML = "Copied!";
            genTimeout = setTimeout(() => {
                genButton.style.backgroundColor = "#008CBA";
                genButton.innerHTML = "Copy to clipboard";
            }, btnTimeout);
        });
    }
}

var manaddTimeout;
function manaddButtonCallback() {
    clearTimeout(manaddTimeout);
    if (manaddLat.value == "" || manaddLng.value == "") {
        manaddButton.style.backgroundColor = "#D21404";
        manaddButton.innerHTML = `Please enter coordinates!`;
        manaddTimeout = setTimeout(() => {
            manaddButton.style.backgroundColor = "#A020F0";
            manaddButton.innerHTML = "Add Waypoint";
        }, btnTimeout);
        return;
    } else {
        map_update(null, manaddLat.value, manaddLng.value);
        manaddButton.style.backgroundColor = "#4CAF50";
        manaddButton.innerHTML = `Added!`;
        manaddTimeout = setTimeout(() => {
            manaddButton.style.backgroundColor = "#A020F0";
            manaddButton.innerHTML = "Add Waypoint";
        }, btnTimeout);
    }
}


/* Begin program execution */

// Attach event listeners to buttons
genButton.addEventListener("click", genButtonCallback);
manaddButton.addEventListener("click", manaddButtonCallback);

// TODO: when you click the screen, put down a waypoint where the entirety of the screen gets greyed out and then you slide to choose altitude
