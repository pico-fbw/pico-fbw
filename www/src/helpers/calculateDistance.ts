/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

export default (lat1: number, lon1: number, lat2: number, lon2: number): string => {
    const earthRadius = 6371e3; // Earth's radius in meters
    const toRadians = (degrees: number) => degrees * (Math.PI / 180);
    const deltaLat = toRadians(lat2 - lat1);
    const deltaLon = toRadians(lon2 - lon1);

    const a =
        Math.sin(deltaLat / 2) * Math.sin(deltaLat / 2) +
        Math.cos(toRadians(lat1)) * Math.cos(toRadians(lat2)) * Math.sin(deltaLon / 2) * Math.sin(deltaLon / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

    const distance = earthRadius * c;
    let result: string;

    if (distance >= 1000) {
        const nauticalMiles = (distance / 1852).toFixed(2); // Conversion: 1 nautical mile = 1852 meters
        result = `${nauticalMiles}nm`;
    } else {
        const roundedDistance = Math.round(distance * 10) / 10; // Round to the nearest tenth
        result = `${roundedDistance}m`;
    }

    return result;
};
