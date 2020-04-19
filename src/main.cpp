#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <ctime>

#include "Outlines.h"
#include "Path.h"

namespace o = Outlines;

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

    Point outlineToPixel(const Outlines::Node& node) const {
        return Point{(node.x - xo) / pathUnitsPerPixel, (node.y - yo) / pathUnitsPerPixel};
    }
};

struct ElementParameters {
    int subSamples;
    Bounds bounds;
    uint32_t onColour;
    uint32_t offColour;
};

class Transform {
    double screenLeftInOutlinePoints;
    double screenTopInOutlinePoints;

    double pixelsPerOutlinePoint;

public:
    void calculateToFit(size_t outlineID, int screenWidth, int screenHeight) {
        int xMin, xMax, yMin, yMax;
        xMin = yMin = std::numeric_limits<int>::max();
        xMax = yMax = std::numeric_limits<int>::min();

        for (auto node = o::ALL_OUTLINES[o::FRAME]; node->action != 'X'; ++node) {
            xMin = std::min(xMin, node->x);
            xMax = std::max(xMax, node->x);
            yMin = std::min(xMin, node->y);
            yMax = std::max(yMax, node->y);
        }

        // Pick the scale that exactly fits to either the width or height but doesn't exceed the other.
        double sx = screenWidth / (static_cast<double>(xMax) - xMin);
        double sy = screenHeight / (static_cast<double>(yMax) - yMin);
        pixelsPerOutlinePoint = std::min(sx, sy);

        screenLeftInOutlinePoints = (static_cast<double>(xMin) + xMax - screenWidth / pixelsPerOutlinePoint) / 2.0;
        screenTopInOutlinePoints = (static_cast<double>(yMin) + yMax - screenHeight /pixelsPerOutlinePoint) / 2.0;
    }

    Point outlineToScreen(const Outlines::Node& outlineNode) {
        return Point{(outlineNode.x - screenLeftInOutlinePoints) * pixelsPerOutlinePoint,
                     (outlineNode.y - screenTopInOutlinePoints) * pixelsPerOutlinePoint};
    }

    Path outlineToPath(size_t outlineID) {
        Path path;

        auto node = Outlines::ALL_OUTLINES[outlineID];

        while (node->action != 'X') {
            if (node->action == 'M') {
                path.moveTo(outlineToScreen(*node));
                ++node;
            } else if (node->action == 'L') {
                path.lineTo(outlineToScreen(*node));
                ++node;
            } else if (node->action == 'A') {
                path.curveTo(outlineToScreen(node[0]), outlineToScreen(node[1]), outlineToScreen(node[2]));
                node += 3;
            }
        }
        return path;
    }
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

        auto node = Outlines::ALL_OUTLINES[outlineID];
        const Bounds& bounds = elementParameters.bounds;

        while (node->action != 'X') {
            if (node->action == 'M') {
                p.moveTo(bounds.outlineToPixel(*node));
                ++node;
            } else if (node->action == 'L') {
                p.lineTo(bounds.outlineToPixel(*node));
                ++node;
            } else if (node->action == 'A') {
                p.curveTo(bounds.outlineToPixel(node[0]), bounds.outlineToPixel(node[1]),
                          bounds.outlineToPixel(node[2]));
                node += 3;
            }
        }

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

    uint32_t offColour = 0x708080;
    uint32_t onColour = 0x424242;

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

protected:
    // see https://en.wikipedia.org/wiki/Seven-segment_display
    static const uint8_t digitToSegments[];

    void renderDigit(SDL_Renderer* renderer, size_t outlineID, size_t digit) {
        uint8_t mask = digitToSegments[digit % 10];
        while (mask != 0) {
            if ((mask & 1) != 0)
                textures[outlineID].render(renderer);
            mask >>= 1;
            outlineID++;
        }
    }

public:
    void setGameColours(uint32_t onColour, uint32_t offColour) {
        this->onColour = onColour;
        this->offColour = offColour;
    }

    void createTextures(SDL_Renderer* renderer, int screenW, int screenH, int subSamples) {
        Worker worker;
        worker.game = this;
        worker.elementParameters.bounds.computeBounds(screenW, screenH);
        worker.elementParameters.onColour = onColour;
        worker.elementParameters.offColour = offColour;

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

    

    void renderTimeMode(SDL_Renderer* renderer) {
        Uint32 currentTicks = SDL_GetTicks();
        Uint32 gamePos = ((currentTicks / 1000) % 22);

        SDL_SetRenderDrawColor(renderer, onColour & 0xFF, (onColour >> 8) & 0xFF, (onColour >> 16) & 0xFF,
            SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        textures[o::FRAME].renderWithInset(renderer, 1);
        textures[o::BODY].render(renderer);
        textures[o::LEFT_ARM].render(renderer);
        textures[o::RIGHT_ARM].render(renderer);

        if (gamePos < 2 || gamePos > 19) {
            textures[o::LEFT_LEG_UP].render(renderer);
            textures[o::RIGHT_LEG_DOWN].render(renderer);
            textures[o::LEFT_ARM_INNER].render(renderer);
            textures[o::RIGHT_ARM_OUTER].render(renderer);
        }
        else if (gamePos >= 9 && gamePos <= 12) {
            textures[o::LEFT_LEG_DOWN].render(renderer);
            textures[o::RIGHT_LEG_UP].render(renderer);
            textures[o::LEFT_ARM_OUTER].render(renderer);
            textures[o::RIGHT_ARM_INNER].render(renderer);
        }
        else {
            textures[o::LEFT_LEG_DOWN].render(renderer);
            textures[o::RIGHT_LEG_DOWN].render(renderer);
            textures[o::LEFT_ARM_MID].render(renderer);
            textures[o::RIGHT_ARM_MID].render(renderer);
        }

        int ballPos = gamePos;
        if (ballPos >= 12)
            ballPos = 22 - ballPos;
        textures[o::OUTER0 + ballPos].render(renderer);

        std::time_t currentTime;
        std::time(&currentTime);
        auto localTime = std::localtime(&currentTime);
        renderDigit(renderer, o::UNIT_A, localTime->tm_min);
        renderDigit(renderer, o::TENS_A, localTime->tm_min / 10);
        int hour = localTime->tm_hour % 12;
        renderDigit(renderer, o::HUND_A, hour);
        if (hour >= 10)
            renderDigit(renderer, o::THOU_A, hour / 10);

        SDL_RenderPresent(renderer);
    }

    void render(SDL_Renderer* renderer, int pos) {
        SDL_SetRenderDrawColor(renderer, onColour & 0xFF, (onColour >> 8) & 0xFF, (onColour >> 16) & 0xFF,
                               SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        textures[o::FRAME].renderWithInset(renderer, 1);
        textures[o::BODY].render(renderer);
        textures[o::LEFT_ARM].render(renderer);
        textures[o::RIGHT_ARM].render(renderer);

        int armPos = (pos / 70) % 4;

        if (armPos == 3)
            armPos = 1;

        int ballPos = (pos / 60) % 22;
        if (ballPos >= 12)
            ballPos = 22 - ballPos;
        textures[o::OUTER0 + ballPos].render(renderer);

        ballPos = (9 + (20 + pos) / 60) % 18;
        if (ballPos >= 10)
            ballPos = 18 - ballPos;
        textures[o::MID0 + ballPos].render(renderer);

        ballPos = ((40 + pos) / 60) % 14;
        if (ballPos >= 8)
            ballPos = 14 - ballPos;
        textures[o::INNER0 + ballPos].render(renderer);

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

        renderDigit(renderer, o::UNIT_A, pos % 10);
        renderDigit(renderer, o::TENS_A, (pos / 10) % 10);
        renderDigit(renderer, o::HUND_A, (pos / 100) % 10);
        renderDigit(renderer, o::THOU_A, (pos / 1000) % 10);

        SDL_RenderPresent(renderer);
    }
};

const uint8_t GameState::digitToSegments[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

class CommandLineParameters {
public:
    int subsamples = 4;
    bool fullscreen = false;
    int width = -1;
    int height = -1;
    int displayIndex = 0;
    uint32_t offColour = 0x708080;
    uint32_t onColour = 0x424242;

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
            }
            else if (std::strcmp(argv[i], "-lcd") == 0 || std::strcmp(argv[i], "-back") == 0) {
                bool isOn = std::strcmp(argv[i], "-lcd") == 0;
                ok = ++i < argc;
                if (ok) {
                    char* end;
                    auto colour = std::strtol(argv[i], &end, 16);
                    if (isOn)
                        onColour = colour;
                    else
                        offColour = colour;
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

    void showUsage() {
        std::cout << "sdlTossup [-f] [-d <display_index>] [-s <subsamples>] [-lcd <hexcolour>] [-back <hexcolour>] [<width> <height>]" << std::endl;
        std::cout << std::endl;
        std::cout << "-f        run game in fullscreen mode." << std::endl;
        std::cout << "-d        display game on the monitor given by <display_index>." << std::endl;
        std::cout << "-s        size <subsamples> by <subsamples> grid used for anti-aliasing. Can be 1 to 15." << std::endl;
        std::cout << "-lcd      the colour in as an BBGGRR hex string for the LCD elements. Defaults to 424242." << std::endl;
        std::cout << "-back     the colour in as an BBGGRR hex string for the screen background. Defaults to 708080." << std::endl;
        std::cout << "<width>   width of the game texture." << std::endl;
        std::cout << "<height>  height of the game texture." << std::endl;
    }

};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    CommandLineParameters parameters;

    if (!parameters.parse(argc, argv)) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        parameters.showUsage();
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
        gameState.setGameColours(parameters.onColour, parameters.offColour);
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

            gameState.renderTimeMode(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}