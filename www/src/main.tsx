/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { render } from "preact";
import { Link, Route, Switch } from "wouter-preact";

import Index from "./pages/Index";
import Planner from "./pages/Planner";
import Settings from "./pages/Settings";
import Upload from "./pages/Upload";

import "./style.css";

function NoMatch() {
    return (
        <div className="flex flex-col items-center justify-center h-screen">
            <h2 className="text-4xl font-bold mb-4 text-gray-300">(404) Nothing to see here!</h2>
            <p>
                <Link to="/" className="text-blue-600 hover:text-sky-500 hover:underline">
                    Return to the home page
                </Link>
            </p>
        </div>
    );
}

export function App() {
    return (
        <main class={"w-full h-full"}>
            <Switch>
                <Route path="/">{() => <Index />}</Route>
                <Route path="/planner">{() => <Planner />}</Route>
                <Route path="/settings">{() => <Settings />}</Route>
                <Route path="/upload">{() => <Upload />}</Route>
                <Route>{() => <NoMatch />}</Route>
            </Switch>
        </main>
    );
}

render(<App />, document.getElementById("root"));

// FEATURE LIST
// [x] Check for (dual-band) internet connection, and if so allow flight planner to be used and data to be uploaded
//   [x] If not, allow uploading externally generated plan (also allow this as an option for dual-band users)
//     [x] Allow both copy/paste and file upload
// [x] Port config editor
// [ ] Allow backing up the config (as a littlefs blob?), to client, and reuploading it
// [ ] Allow saving flightplans to the server (in littlefs)
// [ ] Document most ts functions
// [ ] Add transitions and styling to look cool
// [ ] Allow more actions (retriggering config, rebooting, etc)
// [ ] It also might be fun to have a plane visualization of some sort
// [ ] Easter eggs!!
