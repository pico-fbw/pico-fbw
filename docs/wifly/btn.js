/* Declare constants and vars*/

// Time in milliseconds before a button reverts to its previous state
const btnTimeout = "2500";

const genButton = document.getElementById("generate");
const manaddButton = document.getElementById("manadd-btn");
const manaddLat = document.getElementById("manadd-lat");
const manaddLng = document.getElementById("manadd-lng");

var promptBeforeUnload = false;


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
        changeButton(genButton, "#D21404", "Please select two or more waypoints!");
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
        // Update global status vars
        fplanGenerated = true;
        genButtonCopyState = true;
    } else {
        navigator.clipboard.writeText(field.value).then(() => {
            changeButton(genButton, "#4CAF50", "Copied!");
            genTimeout = setTimeout(() => {
                changeButton(genButton, "#008CBA", "Copy to clipboard");
            }, btnTimeout);
        });
        // Once the flightplan has been copied we know there is no chance of it being lost so we can remove the prompt on close/reload
        promptBeforeUnload = false;
    }
}

var manaddTimeout;

function manaddButtonCallback() {
    clearTimeout(manaddTimeout);
    if (manaddLat.value == "" || manaddLng.value == "") {
        changeButton(manaddButton, "#D21404", "Please enter coordinates!");
        manaddTimeout = setTimeout(() => {
            changeButton(manaddButton, "#A041DB", "Add Waypoint");
        }, btnTimeout);
        return;
    } else {
        map_addWpt(null, manaddLat.value, manaddLng.value);
        // The button is changed in the map.js file so that it changes at the correct time after an alt is set
    }
}


/* Begin program execution */

// Attach event listeners to buttons
genButton.addEventListener("click", genButtonCallback);
manaddButton.addEventListener("click", manaddButtonCallback);

// Attach an event listener to bring up a confirmation dialog when a user tries to leave the page w/o generating
window.onbeforeunload = function() {
    if (promptBeforeUnload) {
        return "";
    } else {
        return;
    }
};
