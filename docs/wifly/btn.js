/* Declare constants and vars*/

// Time in milliseconds before a button reverts to its previous state
const btnTimeout = "3000";

const genButton = document.getElementById("generate");
const manaddButton = document.getElementById("manadd_btn");
const manaddLat = document.getElementById("manadd_lat");
const manaddLng = document.getElementById("manadd_lng");


/* Function definitions */

function changeButton(btn, color, text) {
    btn.style.backgroundColor = color;
    btn.innerHTML = text;
}

var genButtonCopyState = false;
var genTimeout;

function genButtonCallback() {
    // Clear timeout so the button doesn't change back to generate if the user creates waypoints quickly
    clearTimeout(genTimeout);
    if (fplan.waypoints.length < 2) {
        changeButton(genButton, "#D21404", `Please select two or more waypoints (currently ${fplan.waypoints.length})!`);
        genTimeout = setTimeout(() => {
            changeButton(genButton, "#E49B0F", "Generate Flightplan");
        }, btnTimeout);
        return;
    }
    if (!genButtonCopyState) {
        // Generate the flightplan and handle any errors
        if (!wifly_genFplan()) {
            changeButton(genButton, "#D21404", "Flightplan too long!");
            genTimeout = setTimeout(() => {
                changeButton(genButton, "#E49B0F", "Generate Flightplan");
            }, btnTimeout);
            return;
        }
        // Update color and text of button to indicate successful generation
        field.style.display = "block";
        changeButton(genButton, "#008CBA", "Copy to clipboard");
        fplanGenerated = true;
        genButtonCopyState = true;
    } else {
        navigator.clipboard.writeText(field.value).then(() => {
            changeButton(genButton, "#4CAF50", "Copied!");
            genTimeout = setTimeout(() => {
                changeButton(genButton, "#008CBA", "Copy to clipboard");
            }, btnTimeout);
        });
    }
}

var manaddTimeout;

function manaddButtonCallback() {
    clearTimeout(manaddTimeout);
    if (manaddLat.value == "" || manaddLng.value == "") {
        changeButton(manaddButton, "#D21404", "Please enter coordinates!");
        manaddTimeout = setTimeout(() => {
            changeButton(manaddButton, "#A020F0", "Add Waypoint");
        }, btnTimeout);
        return;
    } else {
        map_update(null, manaddLat.value, manaddLng.value);
        changeButton(manaddButton, "#4CAF50", "Added!");
        manaddTimeout = setTimeout(() => {
            changeButton(manaddButton, "#A020F0", "Add Waypoint");
        }, btnTimeout);
    }
}


/* Begin program execution */

// Attach event listeners to buttons
genButton.addEventListener("click", genButtonCallback);
manaddButton.addEventListener("click", manaddButtonCallback);

// TODO: when you click the screen, put down a waypoint where the entirety of the screen gets greyed out and then you slide to choose altitude
