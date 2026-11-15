#pragma once

// Full-canvas static night screen (see app/night_mode.h). Draws over
// the whole display, chrome included — nothing on it changes until
// night mode ends or a button wakes the panel.
void screen_night_render();
