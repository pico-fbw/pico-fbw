/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

var fplan = {
    version: WIFLY_VERSION,
    version_fw: FW_VERSION,
    waypoints: []
};

/* Reference constants */

const field = document.getElementById("fplan");
const tableBody = document.getElementById("waypoints-body");
const altitudeInputs = document.getElementsByClassName("alt-input");
const removeButtons = document.getElementsByClassName("rm-btn");

/* State variable */

var fplanGenerated = false;

/* Global functions */

/**
 * Parses the current flightplan into JSON and places it into the flightplan field.
 * @returns true if generation succeeded, false if the flightplan was too long
 */
function gen_fplan() {
    // Convert the object to JSON string
    var fplanJSON = JSON.stringify(fplan, null, 0);
    // Encode it into a URL temporarily so we can check its length (this is how the Pico will recieve it)
    if (encodeURI(fplanJSON).length > maxFplanLen) {
        return false;
    } else {
        field.value = JSON.stringify(fplan, null, 0);
        gen_wptTable();
        return true;
    }
}

/**
 * Generates the flightplan waypoints table.
 */
function gen_wptTable() {
    // Clear existing rows and repopulate based on the current flightplan list
    tableBody.innerHTML = "";
    fplan.waypoints.forEach((waypoint, index) => {
        var row = document.createElement("tr");
        row.innerHTML = `
            <td>${index + 1}</td>
            <td>${waypoint.lat}, ${waypoint.lng}</td>
            <td>
                <input type="number" class="alt-input" value="${waypoint.alt}" data-index="${index}">
            </td>
            <td>
                <button class="rm-btn" data-index="${index}">Remove</button>
            </td>
        `;
        tableBody.appendChild(row);
    });

    // Add event listeners for altitude input fields and remove buttons (basically make them work)
    Array.from(altitudeInputs).forEach(input => {
        input.addEventListener("input", gen_altChangeCallback);
    });
    Array.from(removeButtons).forEach(button => {
        button.addEventListener("click", btn_removeCallback);
    });
}

/* Local function */

/**
 * Callback for the altitude fields in the flightplan table.
 * @param {Event} event The callback event.
 */
function gen_altChangeCallback(event) {
    var altitude = event.target.value;
    fplan.waypoints[event.target.dataset.index].alt = altitude;
    // Change the generate button's state because a waypoint has been modified
    genButtonState = false;
    btn_change(genButton, state_generate);
    // Enable the unload prompt because a waypoint has been modified
    window_setPromptBeforeUnload(true);
}
