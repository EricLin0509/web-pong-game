#ifndef THEMES_H
#define THEMES_H

typedef struct {
    unsigned char foreground[3];
    unsigned char background[3];
} Theme;

#define THEME_COUNT 10

static const Theme themes[THEME_COUNT] = {
    // Classic-black-white
    { {255, 255, 255}, {0, 0, 0} },
    // Dark-gray-light-gray
    { {231, 235, 237}, {29, 38, 42} },
    // Dark-black-light-white
    { {226, 226, 227}, {44, 46, 52} },
    // Dark-blue-light-cyan
    { {0, 255, 255}, {10, 20, 40} },
    // Dark-blue-light-orange
    { {255, 160, 80}, {20, 25, 45} },
    // Dark-green-light-green
    { {180, 230, 160}, {20, 40, 25} },
    // Dark-violet-champagne-gold
    { {245, 225, 160}, {35, 20, 50} },
    // Dark-brown-light-gray
    { {245, 235, 210}, {40, 30, 20} },
    // Dark-sea-blue-light-blue
    { {180, 220, 255}, {0, 30, 50} },
    // Charcoal-gray-neon-green
    { {120, 255, 120}, {25, 30, 35} }
};

#endif // THEMES_H