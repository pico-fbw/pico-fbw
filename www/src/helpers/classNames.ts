export default (...classes: string[]): string => {
    return classes.filter(Boolean).join(' ');
};
