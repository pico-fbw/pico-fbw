/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

export default (...classes: string[]): string => {
    return classes.filter(Boolean).join(" ");
};
