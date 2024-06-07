/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import preact from "preact";
import { useEffect, useState } from "preact/hooks";
import { useSwipe } from "../helpers/hooks";
import {
    AdjustmentsHorizontalOutline,
    ArrowUpTrayOutline,
    Cog8ToothOutline,
    GlobeAmericasOutline,
    PaperAirplaneOutline,
} from "preact-heroicons";

import Sidebar, { SidebarNavigation } from "./Sidebar";
import Spinner from "./Spinner";

const sidebarNav: SidebarNavigation[] = [
    { name: "Dashboard", to: "/dashboard", icon: PaperAirplaneOutline },
    { name: "Planner", to: "/planner", icon: GlobeAmericasOutline },
    { name: "Upload", to: "/upload", icon: ArrowUpTrayOutline },
    { name: "Settings", to: "/settings", icon: Cog8ToothOutline },
    { name: "Advanced", to: "/advanced", icon: AdjustmentsHorizontalOutline },
];

interface ContentBlockProps {
    title?: string;
    loading?: boolean;
    ignoreSwipe?: boolean;
    children: preact.ComponentChildren;
}

// The ContentBlock is a wrapper for the content of a page, which provides a sidebar for navigation.
const ContentBlock: preact.FunctionComponent<ContentBlockProps> = ({ title, loading, ignoreSwipe, children }) => {
    const [isSidebarOpen, setIsSidebarOpen] = useState(false);

    // Wrap in some swipe handlers to conveniently open/close the sidebar on touchscreen devices
    const swipeHandlers = useSwipe({
        onSwipedLeft: () => !ignoreSwipe && setIsSidebarOpen(false),
        onSwipedRight: () => !ignoreSwipe && setIsSidebarOpen(true),
    });

    useEffect(() => {
        if (title) {
            document.title = `pico-fbw | ${title}`;
        }
    }, [title]);

    return (
        <div {...swipeHandlers} className="h-full">
            <Sidebar navigation={sidebarNav} isOpen={isSidebarOpen} setIsOpen={setIsSidebarOpen} />
            <div className="xl:pl-72 h-full">{loading ? <Spinner /> : <div>{children}</div>}</div>
        </div>
    );
};

export default ContentBlock;
