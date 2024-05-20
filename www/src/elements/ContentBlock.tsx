import preact from "preact";
import { useEffect, useState } from "preact/hooks";
import { useSwipe } from "../helpers/hooks";
import { ArrowUpTrayOutline, Cog8ToothOutline, GlobeAmericasOutline, InformationCircleOutline } from "preact-heroicons";

import Sidebar, { SidebarNavigation } from "./Sidebar";

const sidebarNav: SidebarNavigation[] = [
    { name: "Info", to: "/info", icon: InformationCircleOutline },
    { name: "Planner", to: "/planner", icon: GlobeAmericasOutline },
    { name: "Upload", to: "/upload", icon: ArrowUpTrayOutline },
    { name: "Settings", to: "/settings", icon: Cog8ToothOutline },
];

interface ContentBlockProps {
    title?: string;
    children: preact.ComponentChildren;
}

const ContentBlock: preact.FunctionComponent<ContentBlockProps> = ({ title, children }) => {
    const [isSidebarOpen, setIsSidebarOpen] = useState(false);

    const swipeHandlers = useSwipe({
        onSwipedLeft: () => setIsSidebarOpen(false),
        onSwipedRight: () => setIsSidebarOpen(true),
    });

    useEffect(() => {
        if (title) {
            document.title = `pico-fbw | ${title}`;
        }
    }, [title]);

    return (
        <div {...swipeHandlers} className="h-full">
            <Sidebar navigation={sidebarNav} isOpen={isSidebarOpen} setIsOpen={setIsSidebarOpen} />
            <div className="xl:pl-72">
                <div>{children}</div>
            </div>
        </div>
    );
};

export default ContentBlock;
