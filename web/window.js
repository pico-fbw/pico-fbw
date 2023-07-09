/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

/* Reference constants */

const notify = document.getElementById("notify");
const overlay = document.getElementById("overlay");

/* State variable */

var promptBeforeUnload = false;
var haveBeenPrompted = false;

var notificationTimer;

/* Global function */

/**
 * Displays a notification onscreen to the user (not an alert popup, one that covers the entire page).
 * @param {String} notification Notification to display.
 * @param {boolean} autoClear Whether or not to automatically clear the notification after btnTimeout amount of ms.
 */
function window_displayNotification(notification, autoClear) {
    notifyText.innerHTML = notification;
    overlay.style.display = "block";
    notify.style.display = "block";
    // Clear notification automatically if applicable
    if (autoClear) {
        clearTimeout(notificationTimer);
        notificationTimer = setTimeout(window_hideNotification, btnTimeout);
    }
}

/**
 * Sets the state of prompting the user before unloading the page.
 * @param {Boolean} prompt true to prompt the user, false to not.
 */
function window_setPromptBeforeUnload(prompt) {
    promptBeforeUnload = prompt;
    if (!prompt && haveBeenPrompted) {
        haveBeenPrompted = false;
    }
}

/* Local functions */

/**
 * Hides any notification overlay that may be present onscreen.
 */
function window_hideNotification() {
    overlay.style.display = "none";
    notify.style.display = "none";
}

/**
 * Should be called before the page is about to unload.
 * Prompts the user to rethink their life decisions...
 */
function window_beforeUnload() {
    if (!promptBeforeUnload || haveBeenPrompted) {
        return;
    } else {
        window_displayNotification("You have uncopied changes, click anywhere to return...", false);
        haveBeenPrompted = true;
        return "";
    }
}

/* Runtime code*/

overlay.addEventListener("click", window_hideNotification);
window.onbeforeunload = window_beforeUnload;
