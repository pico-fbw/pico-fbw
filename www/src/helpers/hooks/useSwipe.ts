/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import { useRef } from "preact/hooks";

interface SwipeInput {
    onSwipedLeft: () => void;
    onSwipedRight: () => void;
}

interface SwipeOutput {
    onTouchStart: (e: TouchEvent) => void;
    onTouchMove: (e: TouchEvent) => void;
    onTouchEnd: () => void;
}

/**
 * Hook to detect swipe gestures on a touchscreen device.
 * @param onSwipedLeft the function to call when swiped left
 * @param onSwipedRight the function to call when swiped right
 * @returns the swipe gesture handlers
 */
export default ({ onSwipedLeft, onSwipedRight }: SwipeInput): SwipeOutput => {
    const touchStartX = useRef(0);
    const touchEndX = useRef(0);
    const touchStartY = useRef(0);
    const touchEndY = useRef(0);

    const minSwipeDistance = 50;

    const onTouchStart = (e: TouchEvent) => {
        touchEndX.current = 0;
        touchEndY.current = 0;
        touchStartX.current = e.targetTouches[0].clientX;
        touchStartY.current = e.targetTouches[0].clientY;
    };

    const onTouchMove = (e: TouchEvent) => {
        touchEndX.current = e.targetTouches[0].clientX;
        touchEndY.current = e.targetTouches[0].clientY;
    };

    const onTouchEnd = () => {
        if (!touchStartX.current || !touchEndX.current) {
            return;
        }
        const distanceX = touchStartX.current - touchEndX.current;
        const distanceY = touchStartY.current - touchEndY.current;
        const isVerticalScroll = Math.abs(distanceY) > Math.abs(distanceX);
        if (isVerticalScroll) {
            return;
        }
        const isLeftSwipe = distanceX > minSwipeDistance;
        const isRightSwipe = distanceX < -minSwipeDistance;
        if (isLeftSwipe) {
            onSwipedLeft();
        }
        if (isRightSwipe) {
            onSwipedRight();
        }
    };

    return {
        onTouchStart,
        onTouchMove,
        onTouchEnd,
    };
};
