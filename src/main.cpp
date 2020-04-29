#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>

#include "GameState.h"


namespace o = Outlines;

/*
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
        screenTopInOutlinePoints = (static_cast<double>(yMin) + yMax - screenHeight / pixelsPerOutlinePoint) / 2.0;
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
*/


class CommandLineParameters {
public:
    int subsamples = 4;
    bool fullscreen = false;
    int width = -1;
    int height = -1;
    int displayIndex = 0;
    uint32_t offColour = 0x708080;
    uint32_t onColour = 0x424242;
    bool showInfo = false;

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
            } else if (std::strcmp(argv[i], "-lcd") == 0 || std::strcmp(argv[i], "-back") == 0) {
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
            } else if (std::strcmp(argv[i], "-info") == 0) {
                showInfo = true;
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
        std::cout << "sdlTossup [-f] [-d <display_index>] [-s <subsamples>] [-lcd <hexcolour>] [-back <hexcolour>] "
                     "[<width> <height>]"
                  << std::endl;
        std::cout << std::endl;
        std::cout << "-f        run game in fullscreen mode." << std::endl;
        std::cout << "-d        display game on the monitor given by <display_index>." << std::endl;
        std::cout << "-s        size <subsamples> by <subsamples> grid used for anti-aliasing. Can be 1 to 15."
                  << std::endl;
        std::cout << "-lcd      the colour in as an BBGGRR hex string for the LCD elements. Defaults to 424242."
                  << std::endl;
        std::cout << "-back     the colour in as an BBGGRR hex string for the screen background. Defaults to 708080."
                  << std::endl;
        std::cout << "-info     Show display and audio info and then exit." << std::endl;
        std::cout << "<width>   width of the game texture." << std::endl;
        std::cout << "<height>  height of the game texture." << std::endl;
    }
};

void showInfo() {
    const int displays = SDL_GetNumVideoDisplays();
    std::cout << "Number of displays = " << displays << std::endl;
    for (int display = 0; display < displays; ++display) {
        SDL_Rect displayRect;
        SDL_GetDisplayBounds(display, &displayRect);
        std::cout << "  " << display << ": " << SDL_GetDisplayName(display) << " w = " << displayRect.w
                  << "; h = " << displayRect.h << std::endl;
    }

    const int audioDevices = SDL_GetNumAudioDevices(0);
    std::cout << "Number of audio devices = " << audioDevices << std::endl;
    for (int device = 0; device < audioDevices; ++device) {
        std::cout << "  " << device << ": " << SDL_GetAudioDeviceName(device, 0) << std::endl;
    }
}

class GameSounds {
private:
    struct WavBuffer {
        Uint8* buffer = nullptr;
        Uint32 length = 0;

        bool loadWav(const char* fileName);

        void play(SDL_AudioDeviceID id) {
            SDL_QueueAudio(id, buffer, length);
        }

        ~WavBuffer() {
            if (buffer != nullptr)
                SDL_FreeWAV(buffer);
        }
    };

    WavBuffer innerBuffer;
    WavBuffer midBuffer;
    WavBuffer outerBuffer;
    WavBuffer catchBuffer;
    WavBuffer dropBuffer;
    SDL_AudioDeviceID audioDeviceID = 0;

public:
    GameSounds() = default;

    ~GameSounds();

    bool init();

    void playInnerBeep() {
        innerBuffer.play(audioDeviceID);
    }

    void playMidBeep() {
        midBuffer.play(audioDeviceID);
    }

    void playOuterBeep() {
        outerBuffer.play(audioDeviceID);
    }

    void playDropBeep() {
        dropBuffer.play(audioDeviceID);
    }

    void playCatchBeep() {
        catchBuffer.play(audioDeviceID);
    }
};

bool GameSounds::WavBuffer::loadWav(const char* fileName) {
    SDL_AudioSpec loadedSpec;
    if (SDL_LoadWAV(fileName, &loadedSpec, &buffer, &length) == nullptr) {
        SDL_Log("Failed to load file %s: %s", fileName, SDL_GetError());
        return false;
    }
    return true;
}

GameSounds::~GameSounds() {
    if (audioDeviceID != 0)
        SDL_CloseAudioDevice(audioDeviceID);
}

bool GameSounds::init() {
    if (audioDeviceID != 0) {
        SDL_Log("Error, game sounds are already initialised.");
        return false;
    }

    if (!innerBuffer.loadWav("inner.wav") || !midBuffer.loadWav("mid.wav") || !outerBuffer.loadWav("outer.wav") ||
        !catchBuffer.loadWav("catch.wav") || !dropBuffer.loadWav("drop.wav"))
        return false;

    SDL_AudioSpec desiredSpec = { 0 };
    desiredSpec.freq = 44100;
    desiredSpec.format = AUDIO_S16;
    desiredSpec.channels = 1;
    // We keep the buffer size quite small so ensure any game sounds play without too much delay. in this case 128 / 44.1 ms.
    // By default SDL sets this to 4096 which may result in up a 90ms delay before the sound is played.
    desiredSpec.samples = 128; 

    audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, nullptr, 0);
    if (audioDeviceID == 0) {
        SDL_Log("Could not open audio device: %s", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(audioDeviceID, 0);
    return true;
}

Uint32 playBeep(Uint32 interval, void* param) {
    reinterpret_cast<GameSounds*>(param)->playCatchBeep();
    return 300; 
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    CommandLineParameters parameters;

    if (!parameters.parse(argc, argv)) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        parameters.showUsage();
        return 1;
    }

    if (parameters.showInfo) {
        showInfo();
        return 0;
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

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);  // | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "Error creating renderer:" << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_ShowCursor(SDL_FALSE);

    {
        GameSounds sounds;
        sounds.init();
        GameState gameState;
        gameState.setGameColours(parameters.onColour, parameters.offColour);
        auto s = std::chrono::high_resolution_clock::now();
        gameState.createTextures(renderer, w, h, parameters.subsamples);
        auto e = std::chrono::high_resolution_clock::now();
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
        std::cout << "created textures in: " << delay.count() << "ms." << std::endl;

        //SDL_TimerID timer = SDL_AddTimer(110, playBeep, &sounds);

        bool finished = false;

        gameState.run(renderer);
        //SDL_RemoveTimer(timer);
    }

    

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}