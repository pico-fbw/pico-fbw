/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import calculateDistance from "helpers/calculateDistance";

import { useMapContext } from "./MapContext";

// The table containing all waypoints
export default function WaypointTable() {
    const { markers, markerEditMode } = useMapContext();

    return (
        <table className="min-w-full divide-y divide-gray-700">
            <thead>
                <tr>
                    <th scope="col" className="py-3.5 pl-4 pr-3 text-left text-sm font-semibold text-white sm:pl-0">
                        ID
                    </th>
                    <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                        <div className="hidden md:block">Latitude</div>
                        <div className="md:hidden">Lat.</div>
                    </th>
                    <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                        <div className="hidden md:block">Longitude</div>
                        <div className="md:hidden">Lng.</div>
                    </th>
                    <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                        <div className="hidden md:block">Distance</div>
                        <div className="md:hidden">Dist.</div>
                    </th>
                    <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                        <div className="hidden md:block">Altitude</div>
                        <div className="md:hidden">Alt.</div>
                    </th>
                    <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                        Speed
                    </th>
                    <th scope="col" className="relative py-3.5 pl-3 pr-4 sm:pr-0">
                        <span className="sr-only">Edit</span>
                    </th>
                </tr>
            </thead>
            <tbody className="divide-y divide-gray-800">
                {markers.map((marker, index) => {
                    let distanceToPrevious = "";
                    if (index > 0) {
                        const previousMarker = markers[index - 1];
                        distanceToPrevious = calculateDistance(
                            marker.position.lat,
                            marker.position.lng,
                            previousMarker.position.lat,
                            previousMarker.position.lng,
                        );
                    }

                    return (
                        <tr key={marker.id}>
                            <td className="whitespace-nowrap py-4 pl-4 pr-3 text-sm font-medium text-white sm:pl-0">
                                {marker.id}
                                {marker.drop ? <i> (drop)</i> : ""}
                            </td>
                            <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                <div className="hidden md:block">{marker.position.lat}</div>
                                <div className="md:hidden">{`${marker.position.lat.toFixed(4)}...`}</div>
                            </td>
                            <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                <div className="hidden md:block">{marker.position.lng}</div>
                                <div className="md:hidden">{`${marker.position.lng.toFixed(4)}...`}</div>
                            </td>
                            <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">{distanceToPrevious}</td>
                            <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                <>{marker.alt}ft</>
                            </td>
                            <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                <>{marker.speed}kts</>
                            </td>
                            <td className="relative whitespace-nowrap py-4 pl-3 pr-4 text-right text-sm font-medium sm:pr-0">
                                <a
                                    onClick={() => markerEditMode(marker.id)}
                                    className="text-indigo-400 hover:text-indigo-300 cursor-pointer"
                                >
                                    Edit
                                    <span className="sr-only">, {marker.id}</span>
                                </a>
                            </td>
                        </tr>
                    );
                })}
            </tbody>
        </table>
    );
}
