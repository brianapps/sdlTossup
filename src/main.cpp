#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>

#include "Outlines.h"
#include "Path.h"

void addOutlineToPath(Path& path, int id) {
    auto node = Outlines::ALL_OUTLINES[id];

    while (node->action != 'X') {
        if (node->action == 'M') {
            path.moveTo({node->x, node->y});
            ++node;
        } else if (node->action == 'L') {
            path.lineTo({node->x, node->y});
            ++node;
        } else if (node->action == 'S' || node->action == 'A') {
            path.curveTo({node->x, node->y}, {node[1].x, node[1].y}, {node[2].x, node[2].y});
            node += 3;
        }
    }
}

namespace o = Outlines;
/*
uint32_t LCD_ON = 0x00000000;
uint32_t LCD_OFF = 0x80808000;
*/

class Bounds {
private:
    int xMin, xMax, yMin, yMax;

public:
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

        double sx = (static_cast<double>(xMax) - xMin) / width;
        double sy = (static_cast<double>(yMax) - yMin) / height;

        pathUnitsPerPixel = std::max(sx, sy);
        xo = (static_cast<double>(xMin) + xMax - pathUnitsPerPixel * width) / 2.0;
        yo = (static_cast<double>(yMin) + yMax - pathUnitsPerPixel * height) / 2.0;
    }

    double pixelXToPath(double x) const {
        return x * pathUnitsPerPixel + xo;
    }

    double pathXToPixel(double x) const {
        return (x - xo) / pathUnitsPerPixel;
    }

    double pixelYToPath(double y) const {
        return y * pathUnitsPerPixel + yo;
    }

    double pathYToPixel(double y) const {
        return (y - yo) / pathUnitsPerPixel;
    }


};

struct ElementParameters {
    int subSamples;
    Bounds bounds;
    uint32_t onColour;
    uint32_t offColour;
};


class LcdElementTexture {
protected:
    SDL_Texture* texture = nullptr;
    SDL_Surface* surface = nullptr;
    SDL_Rect dest;

public:
    ~LcdElementTexture() {
        if (texture != nullptr)
            SDL_DestroyTexture(texture);
        if (surface != nullptr)
            SDL_FreeSurface(surface);
    }

    void createSurface(int outlineID, const ElementParameters& elementParameters) {
        if (surface != nullptr)
            SDL_FreeSurface(surface);

        Path p;
        addOutlineToPath(p, outlineID);

        dest.x = static_cast<int>(std::floor(elementParameters.bounds.pathXToPixel(p.leftBound)));
        int right = static_cast<int>(std::ceil(elementParameters.bounds.pathXToPixel(p.rightBound)));
        dest.y = static_cast<int>(std::floor(elementParameters.bounds.pathYToPixel(p.topBound)));
        int bottom = static_cast<int>(std::ceil(elementParameters.bounds.pathYToPixel(p.bottomBound)));

        dest.w = right - dest.x;
        dest.h = bottom - dest.y;

        const double puLeft = elementParameters.bounds.pixelXToPath(dest.x);
        //double puTop = elementParameters.originY + (dest.y * elementParameters.pathUnitsPerPixel);

        surface = SDL_CreateRGBSurfaceWithFormat(0, dest.w, dest.h, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_LockSurface(surface);
        auto row = std::make_unique<uint8_t[]>(dest.w);
        const uint32_t sub2 = elementParameters.subSamples * elementParameters.subSamples;
        for (int y = 0; y < dest.h; ++y) {
            memset(row.get(), 0x00, dest.w);
            for (int subY = 0; subY < elementParameters.subSamples; ++subY) {
                double pathY = elementParameters.bounds.pixelYToPath(y + dest.y + static_cast<double>(subY) / elementParameters.subSamples);
                p.scanLine(pathY, puLeft, elementParameters.bounds.pathUnitsPerPixel, dest.w, elementParameters.subSamples, row.get());
            }
            int* line = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(surface->pixels) + (y * surface->pitch));
            for (int x = 0; x < dest.w; ++x) {
                uint32_t blend1 = (0x100 * row[x]) / sub2;
                uint32_t blend2 = 0x100 - blend1;


                uint32_t cbr = ((elementParameters.onColour & 0xFF00FF) * blend1 + (elementParameters.offColour & 0xFF00FF) * blend2) >> 8;
                uint32_t cg = ((elementParameters.onColour & 0xFF00) * blend1 + (elementParameters.offColour & 0xFF00) * blend2) >> 8;
                uint32_t value = (cbr & 0xFF00FF) | (cg & 0xFF00);


                if (row[x] == 0)
                    line[x] = value;
                else
                    line[x] = 0xFF000000 | value;
            }
        }
        SDL_UnlockSurface(surface);

    }

    void createTexture(SDL_Renderer* renderer) {
        if (texture != nullptr)
            SDL_DestroyTexture(texture);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture == nullptr) {
            std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        }
        SDL_FreeSurface(surface);
        surface = nullptr;
    }

    void setBlendMode(SDL_BlendMode blendMode) {
        SDL_SetTextureBlendMode(texture, blendMode);
    }

    void render(SDL_Renderer* renderer) {
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
    }

    void renderWithInset(SDL_Renderer* renderer, int inset) {
        SDL_Rect s;
        s.x = inset;
        s.y = inset;
        s.w = dest.w - inset * 2;
        s.h = dest.h - inset * 2;
        SDL_Rect d;
        d.x = dest.x + inset;
        d.y = dest.y + inset;
        d.w = s.w;
        d.h = s.h;
        SDL_RenderCopy(renderer, texture, &s, &d);
    }

};



class GameState {
private:
    LcdElementTexture textures[o::COUNT];

    Bounds bounds;

    struct Worker {
        GameState* game;
        ElementParameters elementParameters;
        SDL_Renderer* renderer;
        SDL_atomic_t textureIndex;

        static int start(void* data) {
            return reinterpret_cast<Worker*>(data)->run();
        }

        int run() {
            for (;;) {
                int i = SDL_AtomicAdd(&textureIndex, 1);
                if (i >= o::COUNT)
                    return 0;
                game->textures[i].createSurface(i, elementParameters);
            }
        }
    };

public:
    void createTextures(SDL_Renderer* renderer, int screenW, int screenH, int subSamples) {
        Worker worker;
        worker.game = this;
        worker.elementParameters.bounds.computeBounds(screenW, screenH);
        worker.elementParameters.offColour = 0x808080;
        worker.elementParameters.onColour = 0x101080;

        worker.elementParameters.subSamples = subSamples;
        worker.renderer = renderer;
        SDL_AtomicSet(&worker.textureIndex, 0);
        SDL_Thread* threads[8];
        size_t NUM_THREADS = std::min(SDL_GetCPUCount(), 8);
        std::cout << "Using " << NUM_THREADS << " threads." << std::endl;
        for (size_t i = 0; i < NUM_THREADS; ++i) threads[i] = SDL_CreateThread(Worker::start, "textures", &worker);
        for (size_t i = 0; i < NUM_THREADS; ++i) SDL_WaitThread(threads[i], nullptr);
        for (size_t i = 0; i < o::COUNT; ++i) textures[i].createTexture(renderer);
        textures[o::FRAME].setBlendMode(SDL_BLENDMODE_NONE);
    }

    void drawGame(SDL_Renderer* renderer, int pos) {

        textures[o::FRAME].renderWithInset(renderer, 1);
        textures[o::BODY].render(renderer);
        textures[o::LEFT_ARM].render(renderer);
        textures[o::RIGHT_ARM].render(renderer);

        int armPos = (pos / 70) % 4;

        if (armPos == 3)
            armPos = 1;

        int ballPos = (pos / 50) % 22;
        if (ballPos >= 12)
            ballPos = 22 - ballPos;

        textures[o::OUTER0 + ballPos].render(renderer);

        switch (armPos) {
            case 0:
                textures[o::LEFT_LEG_DOWN].render(renderer);
                textures[o::RIGHT_LEG_UP].render(renderer);
                textures[o::LEFT_ARM_OUTER].render(renderer);
                textures[o::RIGHT_ARM_INNER].render(renderer);
                break;
            case 1:
                textures[o::LEFT_LEG_DOWN].render(renderer);
                textures[o::RIGHT_LEG_DOWN].render(renderer);
                textures[o::LEFT_ARM_MID].render(renderer);
                textures[o::RIGHT_ARM_MID].render(renderer);
                break;
            case 2:
                textures[o::LEFT_LEG_UP].render(renderer);
                textures[o::RIGHT_LEG_DOWN].render(renderer);
                textures[o::LEFT_ARM_INNER].render(renderer);
                textures[o::RIGHT_ARM_OUTER].render(renderer);
                break;
        }
    }
};

class CommandLineParameters {
public:
    int subsamples = 4;
    bool fullscreen = false;
    int width = -1;
    int height = -1;
    int displayIndex = 0;

    bool parse(int argc, char* argv[]) {
        int i = 1;
        int dimension = 0;
        bool ok = true;
        while (ok && i < argc) {
            if (std::strcmp(argv[i], "-f") == 0) {
                fullscreen = true;
            } else if (std::strcmp(argv[i], "-s") == 0) {
                ok = ++i < argc;
                if (ok) {
                    char* end;
                    subsamples = std::strtol(argv[i], &end, 10);
                    ok = *end == '\0';
                }
            } else if (std::strcmp(argv[i], "-d") == 0) {
                ok = ++i < argc;
                if (ok) {
                    char* end;
                    displayIndex = std::strtol(argv[i], &end, 10);
                    ok = *end == '\0';
                }
            } else {
                char* end;
                int number = std::strtol(argv[i], &end, 10);
                ok = *end == '\0' && dimension < 2;
                if (ok) {
                    if (dimension == 0)
                        width = number;
                    else
                        height = number;
                    ++dimension;
                }
            }

            ++i;
        }

        bool areBothDefault = (width == -1 && height == -1);
        bool areNeithDefault = (width != -1) && (height != -1);

        return ok && ((width == -1 && height == -1) || (width != -1 && height != -1));
    }
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    CommandLineParameters parameters;

    if (!parameters.parse(argc, argv)) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        return 1;
    }

    int w, h;
    if (parameters.width == -1) {
        if (parameters.fullscreen) {
            SDL_Rect rect;
            if (SDL_GetDisplayBounds(parameters.displayIndex, &rect) != 0) {
                std::cerr << "Error obtained screen dimensions:" << SDL_GetError() << std::endl;
                return 1;
            }
            w = rect.w;
            h = rect.h;
        } else {
            w = 1024;
            h = 768;
        }
    } else {
        w = parameters.width;
        h = parameters.height;
    }
    // SDL_WINDOWPOS_CENTERED_DISPLAY(parameters.displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(parameters.displayIndex)
    SDL_Window* window =
        SDL_CreateWindow("Test", 0, 0, w, h, (parameters.fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_BORDERLESS));
    if (window == nullptr) {
        std::cerr << "Error creating window:" << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "Error creating renderer:" << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_ShowCursor(SDL_FALSE);

    {
        SDL_Event event;

        GameState gameState;
        auto s = std::chrono::high_resolution_clock::now();
        gameState.createTextures(renderer, w, h, parameters.subsamples);
        auto e = std::chrono::high_resolution_clock::now();
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
        std::cout << "created textures in: " << delay.count() << "ms." << std::endl;

        bool finished = false;

        for (int pos = 0; pos < 6000; ++pos) {
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_KEYDOWN) {
                    finished = finished || event.key.keysym.sym == SDLK_q;
                } else if (event.type == SDL_QUIT) {
                    finished = true;
                }
            }
            if (finished)
                break;

            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderClear(renderer);
            gameState.drawGame(renderer, pos);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}