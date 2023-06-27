/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

// These should always be the same as the versioncodes in version.h!!
const FW_VERSION = "0.01";
const WIFLY_VERSION = "1.0";

var fplan = {
    version: WIFLY_VERSION,
    version_fw: FW_VERSION,
    waypoints: []
};
// Maximum length of the generated flightplan JSON (URL-encoded)
// Please tell me if you're ever actually able to reach this lol
const maxFplanLen = 16384;

const field = document.getElementById("fplan");
const table = document.getElementById("waypoints-body");
const altitudeInputs = document.getElementsByClassName("alt-input");
const removeButtons = document.getElementsByClassName("rm-btn");

var fplanGenerated = false;


/**
 * Generates the current flightplan into JSON and puts it into the fplan field.
 * If the flightplan is too long nothing will be generated.
 * 
 * @returns true if the flightplan was generated, false if it was not.
 */
function wifly_genFplan() {
    // Convert the object to JSON string
    var fplanJSON = JSON.stringify(fplan, null, 0);
    // Encode it into a URL temporarily so we can check its length (this is how the Pico will recieve it)
    if (encodeURI(fplanJSON).length > maxFplanLen) {
        return false;
    } else {
        field.value = JSON.stringify(fplan, null, 0);
        wifly_genWptTable();
        return true;
    }
}

function wifly_genWptTable() {
    table.innerHTML = ""; // Clear existing rows
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
        table.appendChild(row);
    });

    // Add event listeners for altitude input fields
    Array.from(altitudeInputs).forEach(input => {
        input.addEventListener("input", handleAltitudeChange);
    });

    // Add event listeners for remove buttons
    Array.from(removeButtons).forEach(button => {
        button.addEventListener("click", handleRemoveButtonClick);
    });
}

function handleAltitudeChange(event) {
    var altitude = event.target.value;
    fplan.waypoints[event.target.dataset.index].alt = altitude;
    // Change the generate button's state because a waypoint has been modified
    genButtonCopyState = false;
    changeButton(genButton, "#A6710C", "Generate Flightplan");
    // Enable the unload prompt because a waypoint has been modified
    promptBeforeUnload = true;
}
  
function handleRemoveButtonClick(event) {
    map_removeWpt(event.target.dataset.index);
    wifly_genWptTable();
    if (fplanGenerated && fplan.waypoints.length > 0) {
        genButtonCopyState = false;
        changeButton(genButton, "#A6710C", "Generate Flightplan");
    }
    if (fplan.waypoints.length == 0) {
        promptBeforeUnload = false;
    }
}
