#ifndef GAMESTATE_H_
#define GAMESTATE_H_
#include "LcdElement.h"

class GameState {
protected:

    enum class Mode {
        GAME_A, GAME_A_HI_SCORE, GAME_B, GAME_B_HI_SCORE, TIME, ACL
    };

    Mode currentMode = Mode::TIME;

    uint32_t score = 0;

    // Display the juggler figure such that the arms/legs are as follows
    // armPos == 0 - right arm in inner track, left arm in outer track, .
    // armPos == 1 - both arms in the mid track.
    // armPos == 2 - right arm in outer track, left arm in inner track, .

    uint32_t armPosition = 0;
    uint32_t gamePosition = 0;
    bool crashedLeft = false;
    bool crashedRight = false;
    uint32_t gameAHiScore = 139;
    uint32_t gameBHiScore = 970;
    uint32_t timeModeStartedTick = SDL_GetTicks();

    LcdElementTexture textures[Outlines::COUNT];

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
                if (i >= Outlines::COUNT)
                    return 0;
                game->textures[i].createSurface(i, elementParameters);
            }
        }
    };

   

    void renderDigit(SDL_Renderer* renderer, size_t outlineID, size_t digit);


    void renderScore(SDL_Renderer* renderer, uint32_t score);
    void renderTime(SDL_Renderer* renderer);
    void renderGameA(SDL_Renderer* renderer);
    void renderGameB(SDL_Renderer* renderer);
    void startGameA();
    void startGameAHiScore();
    void startGameB();
    void startGameBHiScore();
    void startTimeMode();
    void moveArmsLeft();
    void moveArmsRight();

    bool isRunningGame() {
        return !crashedLeft && !crashedRight && (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B);
    }

    bool isShowingCrashed() {
        return (crashedLeft || crashedRight) && (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B);
    }


public:
    void setGameColours(uint32_t onColour, uint32_t offColour) {
        this->onColour = onColour;
        this->offColour = offColour;
    }

    void createTextures(SDL_Renderer* renderer, int screenW, int screenH, int subSamples);

    void renderFrameAndJuggler(SDL_Renderer* renderer);

    void run(SDL_Renderer* renderer);
};


#endif  // GAMESTATE_H_
