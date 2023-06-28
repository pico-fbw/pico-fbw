/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

// Time in milliseconds before a button reverts to its previous state
const btnTimeout = "2500";

const genButton = document.getElementById("generate");
const manaddButton = document.getElementById("manadd-btn");
const expandTableButton = document.getElementById("expand-table-btn");
const manaddLat = document.getElementById("manadd-lat");
const manaddLng = document.getElementById("manadd-lng");

const waypointsTable = document.getElementById("waypoints-table");

const notify = document.getElementById("notify");
const overlay = document.getElementById("overlay");

var promptBeforeUnload = false;


function changeButton(btn, color, text) {
    btn.style.backgroundColor = color;
    btn.innerHTML = text;
    // Automatically cancel any callbacks that may be occuring just in case the user is doing things quickly
    if (btn == genButton) {
        clearTimeout(genTimeout);
    } else if (btn == manaddButton) {
        clearTimeout(manaddTimeout);
    }
}

var genButtonCopyState = false;
var genTimeout;

function genButtonCallback() {
    if (fplan.waypoints.length < 2) {
        changeButton(genButton, "#D21404", "Please select two or more waypoints!");
        genTimeout = setTimeout(() => {
            changeButton(genButton, "#A6710C", "Generate Flightplan");
        }, btnTimeout);
        return;
    }
    if (!genButtonCopyState) {
        // Generate the flightplan and handle any errors
        if (!wifly_genFplan()) {
            changeButton(genButton, "#D21404", "Flightplan too long!");
            genTimeout = setTimeout(() => {
                changeButton(genButton, "#A6710C", "Generate Flightplan");
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
    if (manaddLat.value == "" || manaddLng.value == "") {
        changeButton(manaddButton, "#D21404", "Please enter coordinates!");
        manaddTimeout = setTimeout(() => {
            changeButton(manaddButton, "#A041DB", "Add Waypoint");
        }, btnTimeout);
        return;
    } else {
        changeButton(manaddButton, "#1520A6", "Adding..");
        map_addWpt(null, manaddLat.value, manaddLng.value);
        // The button is changed in the map.js file so that it changes at the correct time after an alt is set
    }
}

function removeButtonCallback(event) {
    map_removeWpt(event.target.dataset.index);
    wifly_genWptTable();
    if (fplanGenerated && fplan.waypoints.length > 0) {
        genButtonCopyState = false;
        promptBeforeUnload = true;
        changeButton(genButton, "#A6710C", "Generate Flightplan");
    }
    if (fplan.waypoints.length == 0) {
        promptBeforeUnload = false;
    }
}

var notificationTimer;

function displayNotification(notification, autoClear) {
    notifyText.innerHTML = notification;
    overlay.style.display = "block";
    notify.style.display = "block";

    if (autoClear) {
        clearTimeout(notificationTimer);
        notificationTimer = setTimeout(hideNotification, btnTimeout);
    }
}

function hideNotification() {
    overlay.style.display = "none";
    notify.style.display = "none";
}


// Attach event listeners to buttons and overlay
genButton.addEventListener("click", genButtonCallback);
manaddButton.addEventListener("click", manaddButtonCallback);
expandTableButton.addEventListener("click", function() {
    waypointsTable.classList.toggle("expanded");
    expandTableButton.classList.toggle("expanded");
});
overlay.addEventListener("click", hideNotification);

// Attach an event listener to bring up a confirmation dialog when a user tries to leave the page w/o generating
window.onbeforeunload = function() {
    if (promptBeforeUnload) {
        displayNotification("You have uncopied changes, click anywhere to return...", false);
        return "";
    } else {
        return;
    }
};
