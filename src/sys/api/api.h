#pragma once

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
 * @return The status code of the executed command, or 0 if no command was executed
*/
i32 api_poll();
