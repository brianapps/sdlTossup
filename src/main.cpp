#include <SDL.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <algorithm>
#include "Path.h"
#include "Outlines.h"


void addOutlineToPath(Path& path, int id) {
    auto node = Outlines::ALL_OUTLINES[id];

    while (node->action != 'X') {
        if (node->action == 'M') {
            path.moveTo({ node->x, node->y });
            ++node;
        }
        else if (node->action == 'L') {
            path.lineTo({ node->x, node->y });
            ++node;
        }
        else if (node->action == 'S' || node->action == 'A') {
            path.curveTo({ node->x, node->y }, { node[1].x, node[1].y }, { node[2].x, node[2].y });
            node += 3;
        }
    }
}

#ifdef NDEBUG
const int SUB_SAMPLES = 5;
#else
const int SUB_SAMPLES = 2;
#endif

namespace o = Outlines;

class LcdElementTexture {
protected:
    SDL_Texture* texture = nullptr;
    int textureWidth, textureHeight;

    SDL_Rect dest;

public:
    ~LcdElementTexture() {
        if (texture != nullptr) SDL_DestroyTexture(texture);
    }

    void create(SDL_Renderer* renderer, int outlineID) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        Path p;
        addOutlineToPath(p, outlineID);

        double height = p.bottomBound - p.topBound;
        double width = p.rightBound - p.leftBound;

        int pixelH = 800;

        // number of path units per pixel
        double scale = height / (static_cast<double>(pixelH) - 4);
        int pixelW = static_cast<int>(4 + width / scale);

        double puTop = p.topBound - 2 * scale;
        double puLeft = p.leftBound - 2 * scale;

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, pixelW, pixelH, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_LockSurface(surface);
        auto row = std::make_unique<uint8_t[]>(pixelW);
        for (int y = 0; y < pixelH; ++y) {
            memset(row.get(), 0x00, pixelW);
            for (int subY = 0; subY < SUB_SAMPLES; ++subY) {
                p.scanLine(puTop + y * scale + (scale * subY) / SUB_SAMPLES, puLeft, scale, pixelW, SUB_SAMPLES, row.get());
            }
            int* line = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(surface->pixels) + (y * surface->pitch));
            for (int x = 0; x < pixelW; ++x) {
                uint32_t value = 0xFF * row[x] / (SUB_SAMPLES * SUB_SAMPLES);
                line[x] = 0x00000000 | (value << 24);
            }
        }
        SDL_UnlockSurface(surface);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        textureHeight = pixelH;
        textureWidth = pixelW;
        dest.x = 0;
        dest.y = 0;
        dest.w = textureWidth;
        dest.h = textureHeight;
        SDL_FreeSurface(surface);
    }


    void create(SDL_Renderer* renderer, int outlineID, double pathUnitsPerPixel, double originX, double originY) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        Path p;
        addOutlineToPath(p, outlineID);

        dest.x = static_cast<int>(std::floor((p.leftBound - originX) / pathUnitsPerPixel));
        int right = static_cast<int>(std::ceil((p.rightBound - originX) / pathUnitsPerPixel));
        dest.y = static_cast<int>(std::floor((p.topBound - originY) / pathUnitsPerPixel));
        int bottom = static_cast<int>(std::ceil((p.bottomBound - originY) / pathUnitsPerPixel));

        dest.w = right - dest.x;
        dest.h = bottom - dest.y;

        double puLeft = originX + (dest.x * pathUnitsPerPixel);
        double puTop = originY + (dest.y * pathUnitsPerPixel);

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, dest.w, dest.h, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_LockSurface(surface);
        auto row = std::make_unique<uint8_t[]>(dest.w);
        for (int y = 0; y < dest.h; ++y) {
            memset(row.get(), 0x00, dest.w);
            for (int subY = 0; subY < SUB_SAMPLES; ++subY) {
                p.scanLine(puTop + y * pathUnitsPerPixel + (pathUnitsPerPixel * subY) / SUB_SAMPLES, puLeft, pathUnitsPerPixel, dest.w, SUB_SAMPLES, row.get());
            }
            int* line = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(surface->pixels) + (y * surface->pitch));
            for (int x = 0; x < dest.w; ++x) {
                uint32_t value = 0xFF * row[x] / (SUB_SAMPLES * SUB_SAMPLES);
                line[x] = 0x00000000 | (value << 24);
            }
        }
        SDL_UnlockSurface(surface);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }


    void render(SDL_Renderer* renderer) {
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
    }
};


struct Bounds {

    int xMin, xMax, yMin, yMax;
    double pathUnitsPerPixel;
    double xo, yo;


    void computeBounds(int width, int height) {

        xMin = yMin = std::numeric_limits<int>::max();
        xMax = yMax = std::numeric_limits<int>::min();

        for (auto node = o::ALL_OUTLINES[o::FRAME]; node->action != 'X'; ++node) {
            xMin = std::min(xMin, node->x);
            xMax = std::max(xMax, node->x);
            yMin = std::min(xMin, node->y);
            yMax = std::max(yMax, node->y);
        }


        double sx = (xMax - xMin) / static_cast<double>(width);
        double sy = (yMax - yMin) / static_cast<double>(height);

        pathUnitsPerPixel = std::max(sx, sy);
        xo = (static_cast<double>(xMin) + xMax - pathUnitsPerPixel * width) / 2.0;
        yo = (static_cast<double>(yMin) + yMax - pathUnitsPerPixel * height) / 2.0;

    }


};


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);


    Bounds bounds;

    SDL_Window* window = SDL_CreateWindow("Test", 0, 0, 1920, 1080, SDL_WINDOW_BORDERLESS); 
    if (window == nullptr) {
        std::cout << "Error" << std::endl;
        return 1;
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    bounds.computeBounds(w, h);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cout << "renderer error" << std::endl;
        return 1;
    }

    SDL_ShowCursor(SDL_FALSE);

    {
        SDL_Event event;

        LcdElementTexture all[o::COUNT];
        for (size_t i = 0; i < o::COUNT; ++i)
            all[i].create(renderer, i, bounds.pathUnitsPerPixel, bounds.xo, bounds.yo);


        bool finished = false;


        for (int pos = 0; pos < 6000; ++pos) {
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_KEYDOWN) {
                    finished = finished || event.key.keysym.sym == SDLK_q;
                }
                else if (event.type == SDL_QUIT) {
                    finished = true;
                }
            }
            if (finished)
                break;

            int armPos = (pos / 70) % 4;

            if (armPos == 3)
                armPos = 1;

            



            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF);
            SDL_RenderClear(renderer);
            all[o::FRAME].render(renderer);
            all[o::BODY].render(renderer);
            all[o::LEFT_ARM].render(renderer);
            all[o::RIGHT_ARM].render(renderer);


            int ballPos = (pos / 50) % 22;
            if (ballPos >= 12)
                ballPos = 22 - ballPos;


            all[o::OUTER0 + ballPos].render(renderer);

            switch (armPos) {
            case 0:
                all[o::LEFT_LEG_DOWN].render(renderer);
                all[o::RIGHT_LEG_UP].render(renderer);
                all[o::LEFT_ARM_OUTER].render(renderer);
                all[o::RIGHT_ARM_INNER].render(renderer);
                break;
            case 1:
                all[o::LEFT_LEG_DOWN].render(renderer);
                all[o::RIGHT_LEG_DOWN].render(renderer);
                all[o::LEFT_ARM_MID].render(renderer);
                all[o::RIGHT_ARM_MID].render(renderer);
                break;
            case 2:
                all[o::LEFT_LEG_UP].render(renderer);
                all[o::RIGHT_LEG_DOWN].render(renderer);
                all[o::LEFT_ARM_INNER].render(renderer);
                all[o::RIGHT_ARM_OUTER].render(renderer);
                break;
            }
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}