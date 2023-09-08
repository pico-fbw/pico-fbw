#ifndef __MESSAGES_H
#define __MESSAGES_H

#define APP_NAME "pico-fbw Configurator"

#define FIELD_LOAD_PROJECT "Load Project"
#define FIELD_SELECT_MODEL "Select Model"
#define FIELD_BUILD_UPLOAD "Build and Upload"
#define FIELD_EXIT "Exit"

#define FIELD_HELP "Help"
#define FIELD_LICENSES "Licenses"
#define FIELD_ABOUTQT "About Qt"
#define FIELD_ABOUT "About"

#define TIP_LOAD_PROJECT "Loads a project to edit"
#define TIP_SELECT_MODEL "Selects the Pico model to build for"
#define TIP_BUILD_UPLOAD "Builds and uploads the configured firmware to the Pico"

// Note: models must still be manually added to the menu bar, dropdown, and build function logic.
#define MODEL0_NAME "Pico"
#define MODEL0_ARGS "-DPICO_BOARD=pico"
#define MODEL1_NAME "Pico W"
#define MODEL1_ARGS "-DPICO_BOARD=pico_w"

enum Model {
    MODEL0,
    MODEL1
};

#define STEP_DOWNLOAD "<b>Downloading...</b>"
#define STEP_DOWNLOADFINISHED "<b>Download complete!</b>"
#define STEP_BUILD "<b>Building: </b>"
#define STEP_BUILDFINISHED "<b>Build complete!</b>"

#define MSG_ERR "Error"

#define ERR_PROJNOTFOUND "Project not found!"
#define TIP_PROJNOTFOUND "A valid pico-fbw project could not be found.<br><br>What would you like to do?"
#define ERR_PROJINVALID "Invalid project!"
#define TIP_NEWPROJ "Create a new pico-fbw project"
#define TIP_OPENPROJ "Open an existing pico-fbw project"

#define ERR_CFGNOTFOUND "Configuration file not found!"
#define ERR_CFGFAILEDOPEN "Failed to open configuration file!"
#define ERR_CFGFAILEDWRITE "Failed to write configuration file!"

#define ERR_DOWNLOADFAIL "Download failed!"
#define TIP_DOWNLOADFAIL "Failed to download project!<br>Ensure you have all necessary components installed and the directory you selected is empty."

#define ERR_MODELINVALID "Invalid model!"

#define ERR_BUILDFAIL "Build failed!"
#define TIP_BUILDFAIL_CONFIG "Build process failed during configuration step!<br>Ensure you have all necessary components installed and have the correct directory selected."
#define TIP_BUILDFAIL_BUILD "Build process failed during build step!<br>See more details below."
#define MSG_BUILDSUCCESS "Build successful!"
#define TIP_BUILDSUCCESS_UPLOAD "Build process was successful!<br><br>Would you like to upload now?"

#define ERR_UPLOADFAIL "Upload failed!"
#define TIP_UPLOADBNOTFOUND "Firmware build not found!"
#define MSG_UPLOADSUCCESS "Upload successful!"
#define Q_MANUALUPLOAD "Manual upload?"
#define MSG_MANUALUPLOAD "Would you like to upload the firmware manually?"

#define UERR_INVALIDB "Invalid firmware file!"
#define UERR_READFAIL "Failed to read firmware file!"
#define UERR_CONNECTFAIL "Failed to connect to device!"
#define UERR_NODETECT "No device was detected!"
#define UERR_CONNECT "Communication with device failed!"
#define UERR_VERIFY "Verification of firmware failed!"
#if defined(_WIN32)
    #define UERR_PERMS "A device was detected, but the application was unable to communicate with it. Did you try to run the application as administrator?"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #define UERR_PERMS "A device was detected, but the application was unable to communicate with it. Did you try to run the application as root (sudo)?"
#else
    #define UERR_PERMS "A device was detected, but the application was unable to communicate with it. This is likely a permissions issue."
#endif
#define UERR_DRIVER "A device was detected but the application was unable to communicate with it. This is likely a driver issue."
#define UERR_BOOTSEL "A Pico was detected but the application was unable to set it into BOOTSEL mode. Reboot the Pico with the BOOTSEL button pressed and try again."
#define UERR_UNKNOWN "An unknown error occurred. Please try again."

#define MSG_HELP "This application allows you to easily configure, build, and upload the pico-fbw firmware.<br><br>"\
                 "<b>%1</b>: %2<br><br>"\
                 "<b>%3</b>: %4<br><br>"\
                 "<b>%5</b>: %6<br><br>"\
                 "<br>For more information, refer to the online documentation at <a href=\"https://pico-fbw.org\">pico-fbw.org</a>."
#define MSG_LICENSE "This application uses the Qt framework, which is licensed under the GNU GPL-3.0 license.<br><br>"\
                    "The application itself is also licensed under the GNU GPL-3.0 license.<br><br>"\
                    "Parts of this application utilize code from picotool by Raspberry Pi, which is licensed under the BSD 3-Clause license.<br><br>"\
                    "For more details, you can review the license texts:<br>"\
                    "<a href=\"https://www.gnu.org/licenses/gpl-3.0.en.html\">GNU GPL-3.0 License</a><br>"\
                    "<a href=\"https://opensource.org/licenses/BSD-3-Clause\">BSD 3-Clause License</a>"
#define MSG_ABOUT "<b>%1</b> version %2_%3<br><br>"\
                  "This application allows you to easily configure, build, and upload the pico-fbw firmware.<br><br>"\
                  "pico-fbw and this application are created by MylesAndMore and are licensed under the GNU GPL-3.0 license.<br><br>"\
                  "For more information and source code, visit <a href=\"https://pico-fbw.org\">pico-fbw.org</a> or "\
                  "the <a href=\"https://github.com/MylesAndMore/pico-fbw/tree/app\">GitHub repository</a>."

#endif // __MESSAGES_H
