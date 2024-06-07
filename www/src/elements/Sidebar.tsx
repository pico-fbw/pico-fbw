/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import preact from "preact";
import { Bars3MiniSolid, HeroIcon, XMarkOutline } from "preact-heroicons";
import { Link } from "wouter-preact";

import classNames from "../helpers/classNames";

export interface SidebarNavigation {
    name: string;
    to: string;
    icon?: HeroIcon;
}

interface SidebarEntryProps {
    item: SidebarNavigation;
    onClick?: () => void;
}

interface SidebarProps {
    navigation: SidebarNavigation[];
    // isOpen and setIsOpen props are only used for mobile sidebar
    isOpen: boolean;
    setIsOpen: (isOpen: boolean) => void;
}

const SidebarEntry: preact.FunctionComponent<SidebarEntryProps> = ({ item, onClick }) => {
    return (
        <>
            <Link
                to={item.to}
                onClick={onClick}
                className={active =>
                    classNames(
                        active ? "bg-gray-800 text-white" : "text-gray-400 hover:text-white hover:bg-gray-800",
                        "group flex gap-x-3 rounded-md p-2 text-sm leading-6 font-semibold",
                    )
                }
            >
                {item.icon && <item.icon className="h-6 w-6 shrink-0" aria-hidden="true" />}
                {item.name}
            </Link>
        </>
    );
};

const Sidebar: preact.FunctionComponent<SidebarProps> = ({ navigation, isOpen, setIsOpen }) => {
    return (
        <>
            {/* Desktop: static sidebar */}
            <div className="hidden xl:fixed xl:inset-y-0 xl:z-50 xl:flex xl:w-72 xl:flex-col">
                <div className="flex grow flex-col gap-y-5 overflow-y-auto bg-black/10 px-6 ring-1 ring-white/5">
                    <Link to={"/"} className="flex h-[4.5rem] shrink-0 items-center">
                        <img src="icon.svg" className="h-10 w-auto -m-2" />
                    </Link>
                    <nav className="flex flex-1 flex-col">
                        <ul role="list" className="flex flex-1 flex-col gap-y-7">
                            <li>
                                <ul role="list" className="-mx-2 space-y-1">
                                    {navigation.map(item => (
                                        <li key={item.name}>
                                            <SidebarEntry item={item} key={item.name} />
                                        </li>
                                    ))}
                                </ul>
                            </li>
                        </ul>
                    </nav>
                </div>
            </div>
            {/* Mobile: tray icon */}
            <div className="sticky top-0 z-40 flex items-center gap-x-6 bg-gray-900 px-4 py-4 shadow-sm sm:px-6 lg:hidden">
                <button type="button" className="-m-2.5 p-2.5 text-gray-400 lg:hidden" onClick={() => setIsOpen(true)}>
                    <span className="sr-only">Open sidebar</span>
                    <Bars3MiniSolid className="h-6 w-6" aria-hidden="true" />
                </button>
            </div>
            {/* Mobile: full sidebar */}
            <div className="relative z-[2000] lg:hidden">
                <div
                    className={classNames(
                        isOpen ? "block" : "hidden",
                        "fixed inset-0 z-30 bg-black/50 transition-opacity duration-500",
                    )}
                    onClick={() => setIsOpen(false)}
                />
                <div
                    className={classNames(
                        isOpen ? "translate-x-0" : "-translate-x-full",
                        "fixed inset-0 z-40 flex flex-col w-72 bg-gray-900 transform transition-transform duration-300 ease-in-out",
                    )}
                >
                    <div className="flex items-center justify-between px-6 py-4">
                        <Link to="/" className="flex items-center gap-x-2">
                            <img src="icon.svg" className="h-16 w-auto -m-2" />
                        </Link>
                        <button
                            type="button"
                            className="absolute top-0 right-0 m-4 text-gray-400"
                            onClick={() => setIsOpen(false)}
                        >
                            <span className="sr-only">Close sidebar</span>
                            <XMarkOutline className="h-6 w-6" aria-hidden="true" />
                        </button>
                    </div>
                    <nav className="flex flex-col flex-1 overflow-y-auto px-6">
                        <ul role="list" className="flex flex-col flex-1 gap-y-7">
                            <li>
                                <ul role="list" className="-mx-2 space-y-1">
                                    {navigation.map(item => (
                                        <li key={item.name}>
                                            <SidebarEntry item={item} onClick={() => setIsOpen(false)} />
                                        </li>
                                    ))}
                                </ul>
                            </li>
                        </ul>
                    </nav>
                </div>
            </div>
        </>
    );
};

export default Sidebar;
