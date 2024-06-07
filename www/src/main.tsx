/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { render } from "preact";
import { Link, Route, Switch } from "wouter-preact";

import Advanced from "./pages/Advanced";
import Dashboard from "./pages/Dashboard";
import Index from "./pages/Index";
import Planner from "./pages/Planner";
import Settings from "./pages/Settings";
import Upload from "./pages/Upload";

import "./style.css";

// 404 page
function NoMatch() {
    return (
        <div className="flex flex-col items-center justify-center h-screen">
            <h2 className="text-4xl font-bold mb-4 text-gray-300">
                <span className="text-pink-600">(404)</span> Nothing to see here!
            </h2>
            <p>
                <Link to="/" className="text-blue-600 hover:text-sky-500 hover:underline">
                    Return to the home page
                </Link>
            </p>
        </div>
    );
}

function App() {
    return (
        <main class={"w-full h-full"}>
            <Switch>
                <Route path="/">{() => <Index />}</Route>
                <Route path="/advanced">{() => <Advanced />}</Route>
                <Route path="/dashboard">{() => <Dashboard />}</Route>
                <Route path="/planner">{() => <Planner />}</Route>
                <Route path="/settings">{() => <Settings />}</Route>
                <Route path="/upload">{() => <Upload />}</Route>
                <Route>{() => <NoMatch />}</Route>
            </Switch>
        </main>
    );
}

render(<App />, document.getElementById("root"));
