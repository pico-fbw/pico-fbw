/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

// [ ] Allow backing up the config (as a littlefs blob?), to client, and reuploading it
// [ ] Allow more actions (retriggering config, rebooting, etc)

import ContentBlock from "../elements/ContentBlock";

export default function Advanced() {
    return (
        <ContentBlock title="Advanced">
            <div />
        </ContentBlock>
    );
}
