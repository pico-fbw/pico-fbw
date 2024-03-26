import { render } from 'preact';
import { LocationProvider, Router, Route } from 'preact-iso';

import Index from './pages/Index';

import './style.css';

export function App() {
    return (
        <LocationProvider>
            <main class={'w-full h-full'}>
                <Router>
                    <Route path="/" component={Index} />
                    <Route default component={NoMatch} />
                </Router>
            </main>
        </LocationProvider>
    );
}

function NoMatch() {
    return (
        <div className="flex flex-col items-center justify-center h-screen">
            <h2 className="text-4xl font-bold mb-4 text-gray-300">(404) Nothing to see here!</h2>
            <p>
                <a href="/" className="text-blue-600 hover:text-sky-500 hover:underline">
                    Return to the home page
                </a>
            </p>
        </div>
    );
}

render(<App />, document.getElementById('root'));

// TODO: FEATURE LIST
// - Check for (dual-band) internet connection, and if so allow flight planner to be used and data to be uploaded
//   - If not, allow uploading externally generated plan (also allow this as an option for dual-band users)
// - Allow setting config (make an api endpoint /api/v1 where any JSON posted will just be sent to the api)
// - Info page with misc info like version, free heap, etc
// - Allow downloading the entire config (as a littlefs blob?)
