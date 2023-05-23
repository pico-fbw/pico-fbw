/* Declare constants and vars*/

const genButton = document.getElementById("generate");
const regenButton = document.getElementById("regenerate");
const manaddButton = document.getElementById("manadd_btn");
const manaddLat = document.getElementById("manadd_lat");
const manaddLng = document.getElementById("manadd_lng");


/* Function definitions */

var genButtonCopyState = false;
var genTimeout;
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
        fplanGenerated = true;
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
        regenButton.style.visibility = "hidden";
    }, "4000");
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
        }, "4000");
    } else {
        map_update(null, manaddLat.value, manaddLng.value);
        manaddButton.style.backgroundColor = "#4CAF50";
        manaddButton.innerHTML = `Added!`;
        manaddTimeout = setTimeout(() => {
            manaddButton.style.backgroundColor = "#A020F0";
            manaddButton.innerHTML = "Add Waypoint";
        }, "4000");
    }
}


/* Begin program execution */

// Attach event listeners to buttons
genButton.addEventListener("click", genButtonCallback);
regenButton.addEventListener("click", regenButtonCallback);
manaddButton.addEventListener("click", manaddButtonCallback);
