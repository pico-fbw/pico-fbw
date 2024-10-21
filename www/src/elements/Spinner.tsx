/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

const Spinner = () => {
    return (
        <div className="relative top-1/2">
            <div className="absolute left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                <div className="animate-spin rounded-full h-32 w-32 border-t-2 border-b-2 border-white/30" />
            </div>
        </div>
    );
};

export default Spinner;
