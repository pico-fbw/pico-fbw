/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
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

export default ({ onSwipedLeft, onSwipedRight }: SwipeInput): SwipeOutput => {
    const touchStart = useRef(0);
    const touchEnd = useRef(0);

    const minSwipeDistance = 50;

    const onTouchStart = (e: TouchEvent) => {
        touchEnd.current = 0;
        touchStart.current = e.targetTouches[0].clientX;
    };

    const onTouchMove = (e: TouchEvent) => (touchEnd.current = e.targetTouches[0].clientX);

    const onTouchEnd = () => {
        if (!touchStart.current || !touchEnd.current) {
            return;
        }
        const distance = touchStart.current - touchEnd.current;
        const isLeftSwipe = distance > minSwipeDistance;
        const isRightSwipe = distance < -minSwipeDistance;
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
