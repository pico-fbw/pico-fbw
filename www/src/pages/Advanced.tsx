/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// [ ] Allow backing up the config (as a littlefs blob?), to client, and reuploading it
// [ ] Allow more actions (retriggering system/webui setups, rebooting, etc)
// [ ] Add about section w/ version, license, etc

import ContentBlock from "elements/ContentBlock";

export default function Advanced() {
    return (
        <ContentBlock title="Advanced">
            <div />
        </ContentBlock>
    );
}
