#include <SDL.h>
#include <algorithm>
#include <memory>
#include <cstring>

#include "Path.h"
#include "LcdElement.h"
#include "Outlines.h"

namespace o = Outlines;


void Bounds::computeBounds(int width, int height) {
    xMin = yMin = std::numeric_limits<int>::max();
    xMax = yMax = std::numeric_limits<int>::min();

    for (auto node = o::ALL_OUTLINES[o::FRAME]; node->action != 'X'; ++node) {
        xMin = std::min(xMin, node->x);
        xMax = std::max(xMax, node->x);
        yMin = std::min(xMin, node->y);
        yMax = std::max(yMax, node->y);
    }

    double sx = (static_cast<double>(xMax) - xMin) / width;
    double sy = (static_cast<double>(yMax) - yMin) / height;

    pathUnitsPerPixel = std::max(sx, sy);
    xo = (static_cast<double>(xMin) + xMax - pathUnitsPerPixel * width) / 2.0;
    yo = (static_cast<double>(yMin) + yMax - pathUnitsPerPixel * height) / 2.0;
}

void LcdElementTexture::createSurface(int outlineID, const ElementParameters& elementParameters) {
    if (surface != nullptr)
        SDL_FreeSurface(surface);

    Path p;

    auto node = Outlines::ALL_OUTLINES[outlineID];
    const Bounds& bounds = elementParameters.bounds;

    while (node->action != 'X') {
        if (node->action == 'M') {
            p.moveTo(bounds.outlineToPixel(*node));
            ++node;
        }
        else if (node->action == 'L') {
            p.lineTo(bounds.outlineToPixel(*node));
            ++node;
        }
        else if (node->action == 'A') {
            p.curveTo(bounds.outlineToPixel(node[0]), bounds.outlineToPixel(node[1]),
                bounds.outlineToPixel(node[2]));
            node += 3;
        }
    }

    p.end();

    dest.x = static_cast<int>(std::floor(p.leftBound));
    dest.w = static_cast<int>(std::ceil(p.rightBound)) - dest.x;
    dest.y = static_cast<int>(std::floor(p.topBound));
    dest.h = static_cast<int>(std::ceil(p.bottomBound)) - dest.y;

    surface = SDL_CreateRGBSurfaceWithFormat(0, dest.w, dest.h, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surface);
    auto row = std::make_unique<uint8_t[]>(dest.w);
    const uint32_t sub2 = elementParameters.subSamples * elementParameters.subSamples;
    for (size_t y = 0; y < dest.h; ++y) {
        memset(row.get(), 0, dest.w);
        for (size_t subY = 0; subY < elementParameters.subSamples; ++subY) {
            double pathY = dest.y + y + static_cast<double>(subY) / elementParameters.subSamples;
            p.scanLine(pathY, dest.x, 1, dest.w, elementParameters.subSamples, row.get());
        }
        int* line = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(surface->pixels) + (y * surface->pitch));
        for (int x = 0; x < dest.w; ++x) {
            uint32_t blend1 = (0x100 * row[x]) / sub2;
            uint32_t blend2 = 0x100 - blend1;
            uint32_t cbr = ((elementParameters.onColour & 0xFF00FF) * blend1 +
                (elementParameters.offColour & 0xFF00FF) * blend2) >>
                8;
            uint32_t cg = ((elementParameters.onColour & 0xFF00) * blend1 +
                (elementParameters.offColour & 0xFF00) * blend2) >>
                8;
            uint32_t value = (cbr & 0xFF00FF) | (cg & 0xFF00);

            if (row[x] == 0)
                line[x] = value;
            else
                line[x] = 0xFF000000 | value;
        }
    }
    SDL_UnlockSurface(surface);
}

void LcdElementTexture::createTexture(SDL_Renderer* renderer) {
    if (texture != nullptr)
        SDL_DestroyTexture(texture);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
    }
    SDL_FreeSurface(surface);
    surface = nullptr;
}

