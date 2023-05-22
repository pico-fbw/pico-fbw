const field = document.getElementById("fplan");
const genButton = document.getElementById("generate");

/**
 * @returns a Wi-Fly flightplan as a string
 */
function wifly_genFplan() {
    // Create a flightplan object
    var fplan = {
        version: "1.0"
    };
    // Convert the object to JSON string
    return JSON.stringify(fplan, null, 0);
}

var genButtonCopyState = false;
/**
 * Callback function for the generate flightplan button.
 */
function genButtonCallback() {
    if (!genButtonCopyState) {
        // Update the output field with the generated flight plan JSON
        field.value = wifly_genFplan();
        // Update color and text of element to indicate successful generation
        genButton.style.backgroundColor = "#008CBA";
        genButton.innerHTML = "Copy to clipboard";
        genButtonCopyState = true;
    } else {
        const cb = navigator.clipboard;
        cb.writeText(field.value).then(() => {
            genButton.style.backgroundColor = "#4CAF50";
            genButton.innerHTML = "Copied!";
        });
    }
}

// Attach event listener to the generate button
genButton.addEventListener("click", genButtonCallback);
