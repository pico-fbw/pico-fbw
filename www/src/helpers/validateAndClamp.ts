/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

/**
 * Validates a number and clamps it to a given range.
 * @param value the value to validate
 * @param min the minimum value
 * @param max the maximum value
 * @returns the validated and clamped value
 */
export default (value: number, min: number, max: number): number => {
    if (isNaN(value)) {
        return min;
    }
    return Math.min(Math.max(value, min), max);
};
