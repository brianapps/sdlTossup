#include <SDL.h>
#include <iostream>
#include <memory>
#include <cstring>
#include "Path.h"
#include "outlines.h"


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


const int SUB_SAMPLES = 4;

namespace o = Outlines;

class LcdElementTexture {
protected:
    SDL_Texture* texture = nullptr;
    int textureWidth, textureHeight;

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
        SDL_FreeSurface(surface);
    }

    void render(SDL_Renderer* renderer, int p) {
        SDL_Rect dest;
        dest.x = p;
        dest.y = p / 2;
        dest.w = textureWidth;
        dest.h = textureHeight;
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
    }
};


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDL_Window* window = SDL_CreateWindow("Test", 0, 0, 1920, 1080, SDL_WINDOW_BORDERLESS); 
    if (window == nullptr) {
        std::cout << "Error" << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cout << "renderer error" << std::endl;
        return 1;
    }

    SDL_ShowCursor(SDL_FALSE);

    {
        LcdElementTexture body;
        body.create(renderer, o::LEFT_SPLAT);
        SDL_Event event;

        bool finished = false;


        for (int pos = 0; pos < 600; ++pos) {
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
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF);
            SDL_RenderClear(renderer);
            body.render(renderer, 1);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}