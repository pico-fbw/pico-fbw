/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

// These should always be the same as the versioncodes in version.h!!
// They are placed directly into the flightplan when generated
const FW_VERSION = "0.0.1";
const WIFLY_VERSION = "1.0";

// Maximum length of the generated flightplan (JSON, URL-encoded)
// Please tell me if you're ever actually able to reach this lol
const maxFplanLen = 16384;

// Time in milliseconds before a button reverts to its previous state, also used in the notification overlay
const btnTimeout = "2500";

// Color of the lines drawn between waypoints on the map
const polyLineColor = "#D21404";
