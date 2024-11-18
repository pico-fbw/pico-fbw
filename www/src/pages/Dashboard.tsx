/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

// [ ] General dashboard layout with recomended actions to take based on current state
// [ ] It also might be fun to have a plane visualization of some sort, and be able to click on parts to see details or control things

import { useEffect, useState } from "preact/hooks";

import Alert from "../elements/Alert";
import ContentBlock from "../elements/ContentBlock";

import { api, GET_LOGS } from "../helpers/api";

export default function Dashboard() {
    const [logs, setLogs] = useState<GET_LOGS | null>(null);

    /**
     * Convert an API log type to an alert type.
     * @param type API log type
     * @returns alert type
     */
    const logTypeToAlertType = (type: number) => {
        switch (type) {
            case 1:
                return "info";
            case 2:
                return "warning";
            case 3:
                return "danger";
        }
    };

    useEffect(() => {
        api("get/logs")
            .then(data => {
                setLogs(data);
            })
            .catch(console.error);
    }, []);

    return (
        <ContentBlock title="Dashboard">
            {logs && logs.logs.length > 0 && (
                <div className="max-w-7xl mx-auto mt-4">
                    {logs.logs.map((log, i) => (
                        <Alert
                            key={i}
                            type={logTypeToAlertType(log.type)}
                            onClose={() => {}}
                            className="flex mx-4 sm:mx-8 lg:mx-0 mb-2"
                        >
                            <strong>{log.msg}</strong>
                            {log.code > 0 && <>&nbsp;(FBW-{log.code})</>}
                        </Alert>
                    ))}
                </div>
            )}
        </ContentBlock>
    );
}
