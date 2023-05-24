/* Declare constants and vars*/

var fplan = {
    // Versioncode is here, change when there is a new release
    version: "0.1a",
    waypoints: []
};
// Maximum length of the generated flightplan JSON (URL-encoded)
// Please tell me if you're ever actually able to reach this lol
const maxFplanLen = 16384;

const field = document.getElementById("fplan");


/* Function definitions */

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
        return true;
    }
}

// TODO: a user-readable way to display the generated flightplan, edit altitudes, etc.
