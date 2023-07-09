/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

/* Reference constants */

const genButton = document.getElementById("generate");
const manaddButton = document.getElementById("manadd-btn");
const expandTableButton = document.getElementById("expand-table-btn");

const manaddLat = document.getElementById("manadd-lat");
const manaddLng = document.getElementById("manadd-lng");

const waypointsTable = document.getElementById("waypoints-table");

// For generate button
const state_moreWpts = 0;
const state_tooLong = 1;
const state_generate = 2;
const state_copy = 3;
const state_copied = 4;

const btnState_generate = 0;
const btnState_copy = 1;

// For manadd button
const state_enterCoords = 5;
const state_addWpt = 6;
const state_adding = 7;
const state_added = 8;

/* State variables */

var genButtonState = btnState_generate;

var genTimeout;
var manaddTimeout;

/* Global functions */

/**
 * Changes a specified button to a specified state.
 * @param {HTMLElement} btn The button to change.
 * @param {*} state The state (from the "state_" constants) to apply.
 */
function btn_change(btn, state) {
    switch (state) {
        case state_moreWpts:
            color = "#D21404";
            text = "Please select two or more waypoints!";
            break;
        case state_tooLong:
            color = "#D21404";
            text = "Flightplan too long!";
            break;
        case state_generate:
            color = "#A6710C";
            text = "Generate Flightplan";
            break;
        case state_copy:
            color = "#008CBA";
            text = "Copy to clipboard";
            break;
        case state_copied:
            color = "#4CAF50";
            text = "Copied!";
            break;
        case state_enterCoords:
            color = "#D21404";
            text = "Please enter coordinates!";
            break;
        case state_addWpt:
            color = "#A041DB";
            text = "Add Waypoint";
            break;
        case state_adding:
            color = "#1520A6";
            text = "Adding..";
            break;
        case state_added:
            color = "#4CAF50";
            text = "Added!";
            break;
            
        default:
            return;
    }
    btn.style.backgroundColor = color;
    btn.innerHTML = text;
    // Automatically cancel any callbacks that may be occuring (in case the user is doing things quickly, the buttons glitch out)
    if (btn == genButton) {
        clearTimeout(genTimeout);
    } else if (btn == manaddButton) {
        clearTimeout(manaddTimeout);
    }
}

/**
 * Callback for the remove buttons in the flightplan table.
 * Removes the waypoint (duh) and updates applicable values throughout the program.
 * @param {Event} event The callback event.
 */
function btn_removeCallback(event) {
    // Remove the waypoint (duh), regenerate the table, und update status variables for button/flightplan/window as necessary
    map_removeWpt(event.target.dataset.index);
    gen_wptTable();
    if (fplanGenerated && fplan.waypoints.length > 0) {
        genButtonState = btnState_generate;
        window_setPromptBeforeUnload(true);
        btn_change(genButton, state_generate);
    }
    if (fplan.waypoints.length == 0) {
        window_setPromptBeforeUnload(false);
    }
}

/* Local functions */

/**
 * Callback for the generate button.
 * Generates or copies the flightplan where applicable.
 */
function btn_genCallback() {
    // Don't allow generating a flightplan with less than two waypoints
    if (fplan.waypoints.length < 2) {
        btn_change(genButton, state_moreWpts);
        genTimeout = setTimeout(() => {
            btn_change(genButton, state_generate);
        }, btnTimeout);
        return;
    }
    // Either generate flightplan or copy, depending on state of flightplan
    if (genButtonState == btnState_generate) {
        // Generate the flightplan and handle any errors
        if (!gen_fplan()) {
            btn_change(genButton, state_moreWpts);
            genTimeout = setTimeout(() => {
                btn_change(genButton, state_generate);
            }, btnTimeout);
            return;
        }
        // Update color and text of button to indicate successful generation
        field.style.display = "block";
        btn_change(genButton, state_copy);
        // Update global status vars
        fplanGenerated = true;
        genButtonState = btnState_copy;
    } else {
        navigator.clipboard.writeText(field.value).then(() => {
            btn_change(genButton, state_copied);
            genTimeout = setTimeout(() => {
                btn_change(genButton, state_copy);
            }, btnTimeout);
        });
        // Once the flightplan has been copied we know there is no chance of it being lost so we can remove the prompt on close/reload
        window_setPromptBeforeUnload(false);
    }
}

/**
 * Callback for the manual waypoint add button.
 * Adds the specified to the map/flightplan when possible.
 */
function btn_manaddCallback() {
    if (manaddLat.value == "" || manaddLng.value == "") {
        btn_change(manaddButton, state_enterCoords);
        manaddTimeout = setTimeout(() => {
            btn_change(manaddButton, state_addWpt);
        }, btnTimeout);
        return;
    } else {
        // The button is changed in map.js so that it changes at the correct times
        map_addWpt(null, manaddLat.value, manaddLng.value);
    }
}

/* Runtime code*/

// Attach event listeners to buttons, overlay, and window
genButton.addEventListener("click", btn_genCallback);
manaddButton.addEventListener("click", btn_manaddCallback);
expandTableButton.addEventListener("click", function() {
    waypointsTable.classList.toggle("expanded");
    expandTableButton.classList.toggle("expanded");
});
