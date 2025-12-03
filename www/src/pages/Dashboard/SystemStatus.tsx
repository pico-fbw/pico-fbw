/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import {
    ExclamationTriangleOutline,
    CheckCircleOutline,
    InformationCircleOutline,
    SignalOutline,
    BoltOutline,
    CpuChipOutline,
} from "preact-heroicons";
import classNames from "helpers/classNames";
import { GET_SENSOR, GET_MODE, GET_INFO, GET_LOGS } from "helpers/apiTypes";

interface SystemStatusProps {
    sensorData: GET_SENSOR | null;
    modeData: GET_MODE | null;
    infoData: GET_INFO | null;
    logsData: GET_LOGS | null;
}

interface Alert {
    id: string;
    type: "info" | "warning" | "error";
    icon: any;
    title: string;
    message: string;
    action?: string;
}

export default function SystemStatus({ sensorData, modeData, infoData, logsData }: SystemStatusProps) {
    const generateAlerts = (): Alert[] => {
        const alerts: Alert[] = [];

        // GPS
        if (sensorData?.gps) {
            const sats = sensorData.gps.sats;
            const hdop = sensorData.gps.hdop;
            if (sats < 4) {
                alerts.push({
                    id: "gps-weak",
                    type: "warning",
                    icon: SignalOutline,
                    title: "GPS Signal Weak",
                    message: `Only ${sats} satellite${sats !== 1 ? "s" : ""} in view (minimum 4 recommended).`,
                    action: "Move to open area for better satellite reception.",
                });
            }
            if (hdop > 5) {
                alerts.push({
                    id: "gps-poor-accuracy",
                    type: "info",
                    icon: SignalOutline,
                    title: "GPS Accuracy Reduced",
                    message: `HDOP value of ${hdop.toFixed(1)} indicates reduced position accuracy.`,
                    action: "Wait for better satellite geometry or relocate if possible.",
                });
            }
        }

        // Battery
        if (sensorData?.batt && sensorData.batt.length > 0) {
            const cells = sensorData.batt;
            const avgVoltage = cells.reduce((a, b) => a + b, 0) / cells.length;
            const minVoltage = Math.min(...cells);
            const maxVoltage = Math.max(...cells);
            const voltageSpread = maxVoltage - minVoltage;

            if (avgVoltage < 3.3) {
                alerts.push({
                    id: "battery-critical",
                    type: "error",
                    icon: BoltOutline,
                    title: "Battery Voltage Critical",
                    message: `Average cell voltage: ${avgVoltage.toFixed(2)}V.`,
                    action: "Land immediately and charge battery. Do not continue flight.",
                });
            } else if (avgVoltage < 3.5) {
                alerts.push({
                    id: "battery-low",
                    type: "warning",
                    icon: BoltOutline,
                    title: "Battery Voltage Low",
                    message: `Average cell voltage: ${avgVoltage.toFixed(2)}V.`,
                    action: "Consider landing soon. Charge battery upon landing.",
                });
            }

            if (voltageSpread > 0.1) {
                alerts.push({
                    id: "battery-imbalance",
                    type: "warning",
                    icon: BoltOutline,
                    title: "Battery Cell Imbalance",
                    message: `Voltage spread: ${voltageSpread.toFixed(2)}V between cells.`,
                    action: "Balance charge battery after flight. Cell imbalance can be unsafe.",
                });
            }
        }

        // IMU
        if (sensorData?.imu) {
            if (sensorData.imu.roll === null) {
                alerts.push({
                    id: "imu-failure",
                    type: "error",
                    icon: CpuChipOutline,
                    title: "IMU Sensor Failure",
                    message: "Inertial Measurement Unit not responding.",
                    action: "Check sensor connections and restart system. Do not fly without IMU.",
                });
            }
        }

        // Storage
        if (infoData) {
            const usagePercent = (infoData.fs_used / infoData.fs_total) * 100;

            if (usagePercent > 90) {
                alerts.push({
                    id: "storage-critical",
                    type: "error",
                    icon: InformationCircleOutline,
                    title: "Storage Nearly Full",
                    message: `${usagePercent.toFixed(1)}% of storage capacity used.`,
                    action: "Delete old flight plans to free up space.",
                });
            } else if (usagePercent > 75) {
                alerts.push({
                    id: "storage-warning",
                    type: "warning",
                    icon: InformationCircleOutline,
                    title: "Storage Space Low",
                    message: `${usagePercent.toFixed(1)}% of storage capacity used.`,
                    action: "Consider cleaning up old flight plans to prevent storage issues.",
                });
            }
        }

        // Mode
        if (modeData) {
            if (modeData.mode === "launch") {
                alerts.push({
                    id: "mode-launch",
                    type: "info",
                    icon: CheckCircleOutline,
                    title: "Launch Assist Active",
                    message: "Launch assist mode is engaged.",
                    action: "Throw aircraft forward firmly. System will stabilize automatically.",
                });
            } else if (modeData.mode === "tune") {
                alerts.push({
                    id: "mode-tune",
                    type: "info",
                    icon: CpuChipOutline,
                    title: "Auto-Tune Mode Active",
                    message: "PID auto-tuning in progress.",
                    action: "Perform slow, controlled maneuvers. Avoid aggressive inputs.",
                });
            }
        }

        // Logs (direct from API)
        if (logsData && logsData.logs.length > 0) {
            const recentErrors = logsData.logs.filter(log => log.type === 3).slice(0, 2);
            const recentWarnings = logsData.logs.filter(log => log.type === 2).slice(0, 1);
            const recentInfos = logsData.logs.filter(log => log.type === 1).slice(0, 1);

            recentErrors.forEach((log, idx) => {
                alerts.push({
                    id: `log-error-${idx}`,
                    type: "error",
                    icon: ExclamationTriangleOutline,
                    title: "System Error",
                    message: log.msg,
                    action: `Error code: FBW-${log.code}.`,
                });
            });

            recentWarnings.forEach((log, idx) => {
                alerts.push({
                    id: `log-warning-${idx}`,
                    type: "warning",
                    icon: ExclamationTriangleOutline,
                    title: "System Warning",
                    message: log.msg,
                    action: `Warning code: FBW-${log.code}`,
                });
            });

            recentInfos.forEach((log, idx) => {
                alerts.push({
                    id: `log-info-${idx}`,
                    type: "info",
                    icon: InformationCircleOutline,
                    title: "System Info",
                    message: log.msg,
                    action: log.code > 0 ? `Info code: FBW-${log.code}` : undefined,
                });
            });
        }

        // If no alerts, show all clear message
        if (alerts.length === 0) {
            alerts.push({
                id: "all-clear",
                type: "info",
                icon: CheckCircleOutline,
                title: "All Systems Nominal",
                message: "No issues detected. Aircraft is ready for flight.",
            });
        }

        return alerts;
    };

    const alerts = generateAlerts();

    const getAlertColors = (type: Alert["type"]) => {
        switch (type) {
            case "error":
                return {
                    bg: "bg-red-500/10",
                    border: "border-red-500/30",
                    icon: "text-red-500",
                    title: "text-red-400",
                };
            case "warning":
                return {
                    bg: "bg-yellow-500/10",
                    border: "border-yellow-500/30",
                    icon: "text-yellow-500",
                    title: "text-yellow-400",
                };
            case "info":
                return {
                    bg: "bg-blue-500/10",
                    border: "border-blue-500/30",
                    icon: "text-blue-500",
                    title: "text-blue-400",
                };
        }
    };

    return (
        <div className="bg-gray-800 rounded-lg p-6">
            <h3 className="text-xl font-semibold text-white mb-4">Alerts & Recommendations</h3>
            <div className="space-y-3">
                {alerts.map(alert => {
                    const colors = getAlertColors(alert.type);
                    const Icon = alert.icon;

                    return (
                        <div
                            key={alert.id}
                            className={classNames(
                                "rounded-lg border-2 p-4 transition-all duration-200",
                                colors.bg,
                                colors.border,
                            )}
                        >
                            <div className="flex items-start space-x-3">
                                <Icon className={classNames("h-6 w-6 flex-shrink-0 mt-0.5", colors.icon)} />
                                <div className="flex-1 min-w-0">
                                    <h4 className={classNames("text-sm font-semibold mb-1", colors.title)}>
                                        {alert.title}
                                    </h4>
                                    <p className="text-sm text-gray-300 mb-2">{alert.message}</p>
                                    {alert.action && (
                                        <div className="flex space-x-2 mt-2">
                                            <InformationCircleOutline className="h-4 w-4 text-gray-400 flex-shrink-0 my-auto" />
                                            <p className="text-sm text-gray-400 italic my-auto">{alert.action}</p>
                                        </div>
                                    )}
                                </div>
                            </div>
                        </div>
                    );
                })}
            </div>
        </div>
    );
}
