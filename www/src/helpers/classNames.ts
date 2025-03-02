/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

/**
 * Combines multiple classes into a single string.
 * @param classes the classes to combine
 * @returns the combined classes
 */
export default (...classes: string[]): string => {
    return classes.filter(Boolean).join(" ");
};
