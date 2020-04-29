#include <SDL.h>
#include <algorithm>
#include <ctime>

#include "GameState.h"
#include "LcdElement.h"

namespace {
    // see https://en.wikipedia.org/wiki/Seven-segment_display
    const uint8_t digitToSegments[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
}



void GameState::renderDigit(SDL_Renderer* renderer, size_t outlineID, size_t digit) {
    uint8_t mask = digitToSegments[digit % 10];
    while (mask != 0) {
        if ((mask & 1) != 0)
            textures[outlineID].render(renderer);
        mask >>= 1;
        outlineID++;
    }
}

 void GameState::createTextures(SDL_Renderer* renderer, int screenW, int screenH, int subSamples) {
    Worker worker;
    worker.game = this;
    worker.elementParameters.bounds.computeBounds(screenW, screenH);
    worker.elementParameters.onColour = onColour;
    worker.elementParameters.offColour = offColour;

    worker.elementParameters.subSamples = subSamples;
    worker.renderer = renderer;
    SDL_AtomicSet(&worker.textureIndex, 0);
    SDL_Thread* threads[8];
    size_t numberOfThreads = std::min(SDL_GetCPUCount(), 8);
    SDL_Log("Using %d  threads.", numberOfThreads);
    for (size_t i = 0; i < numberOfThreads; ++i) threads[i] = SDL_CreateThread(Worker::start, "textures", &worker);
    for (size_t i = 0; i < numberOfThreads; ++i) SDL_WaitThread(threads[i], nullptr);
    for (size_t i = 0; i < Outlines::COUNT; ++i) textures[i].createTexture(renderer);
    textures[Outlines::FRAME].setBlendMode(SDL_BLENDMODE_NONE);
}

void GameState::renderFrameAndJuggler(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, onColour & 0xFF, (onColour >> 8) & 0xFF, (onColour >> 16) & 0xFF,
        SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    textures[Outlines::FRAME].renderWithInset(renderer, 1);
    textures[Outlines::BODY].render(renderer);
    textures[Outlines::LEFT_ARM].render(renderer);
    textures[Outlines::RIGHT_ARM].render(renderer);
    switch (armPosition % 3) {
    case 0:
        textures[Outlines::LEFT_LEG_DOWN].render(renderer);
        textures[Outlines::RIGHT_LEG_UP].render(renderer);
        textures[Outlines::LEFT_ARM_OUTER].render(renderer);
        textures[Outlines::RIGHT_ARM_INNER].render(renderer);
        break;
    case 1:
        textures[Outlines::LEFT_LEG_DOWN].render(renderer);
        textures[Outlines::RIGHT_LEG_DOWN].render(renderer);
        textures[Outlines::LEFT_ARM_MID].render(renderer);
        textures[Outlines::RIGHT_ARM_MID].render(renderer);
        break;
    case 2:
        textures[Outlines::LEFT_LEG_UP].render(renderer);
        textures[Outlines::RIGHT_LEG_DOWN].render(renderer);
        textures[Outlines::LEFT_ARM_INNER].render(renderer);
        textures[Outlines::RIGHT_ARM_OUTER].render(renderer);
        break;
    }

    if (crashedLeft) {
        textures[Outlines::LEFT_SPLAT].render(renderer);
        textures[Outlines::LEFT_CRUSH].render(renderer);
    }

    if (crashedRight) {
        textures[Outlines::RIGHT_SPLAT].render(renderer);
        textures[Outlines::RIGHT_CRUSH].render(renderer);
    }

}

void GameState::renderScore(SDL_Renderer* renderer, uint32_t score) {
    renderDigit(renderer, Outlines::UNIT_A, score);
    if (score >= 10) {
        renderDigit(renderer, Outlines::TENS_A, score / 10);
        if (score >= 100) {
            renderDigit(renderer, Outlines::HUND_A, score / 100);
            if (score >= 1000) {
                renderDigit(renderer, Outlines::THOU_A, score / 1000);
            }
        }
    }
}

void GameState::renderTime(SDL_Renderer* renderer) {
    Uint32 currentTicks = SDL_GetTicks() - timeModeStartedTick;
    Uint32 gamePos = ((11 + currentTicks / 1000) % 22);
    if (gamePos < 2 || gamePos > 19)
        armPosition = 2;
    else if (gamePos >= 9 && gamePos <= 12)
        armPosition = 0;
    else
        armPosition = 1;

    renderFrameAndJuggler(renderer);

    int ballPos = gamePos;
    if (ballPos >= 12)
        ballPos = 22 - ballPos;
    textures[Outlines::OUTER0 + ballPos].render(renderer);

    std::time_t currentTime;
    std::time(&currentTime);
    auto localTime = std::localtime(&currentTime);
    renderScore(renderer, (localTime->tm_hour % 12) * 100 + localTime->tm_min);
    SDL_RenderPresent(renderer);
}


void GameState::renderGameA(SDL_Renderer* renderer) {
    renderFrameAndJuggler(renderer);
    renderScore(renderer, score);
    SDL_RenderPresent(renderer);
}


void GameState::renderGameB(SDL_Renderer* renderer) {
    renderFrameAndJuggler(renderer);
    renderScore(renderer, score);
    SDL_RenderPresent(renderer);
}



void GameState::startGameA() {
    if (!isRunningGame()) {
        currentMode = Mode::GAME_A;
        score = 0;
        crashedLeft = false;
        crashedRight = false;
    }
}

void GameState::startGameAHiScore() {
    if (!isRunningGame()) {
        currentMode = Mode::GAME_A_HI_SCORE;
        score = gameAHiScore;
        crashedLeft = false;
        crashedRight = false;
    }
}

void GameState::startGameB() {
    if (!isRunningGame()) {
        currentMode = Mode::GAME_B;
        score = 0;
        crashedLeft = false;
        crashedRight = false;
    }
}

void GameState::startGameBHiScore() {
    if (!isRunningGame()) {
        currentMode = Mode::GAME_B_HI_SCORE;
        score = gameBHiScore;
        crashedLeft = false;
        crashedRight = false;
    }

}

void GameState::startTimeMode() {
    if (isShowingCrashed()) {
        crashedLeft = false;
        crashedRight = false;
        timeModeStartedTick = SDL_GetTicks();
        currentMode = Mode::TIME;
    }
}

void GameState::moveArmsRight() {
    if (armPosition < 2 && (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B))
        armPosition++;
}

void GameState::moveArmsLeft() {
    if (armPosition > 0 && (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B))
        armPosition--;
}


void GameState::run(SDL_Renderer* renderer) {
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_x:
                    return;
                case SDLK_q:
                    moveArmsLeft();
                    break;
                case SDLK_p:
                    moveArmsRight();
                    break;
                case SDLK_a:
                    startGameAHiScore();
                    break;
                case SDLK_b:
                    startGameBHiScore();
                    break;
                case SDLK_t:
                    startTimeMode();
                    break;
                case SDLK_w:
                    crashedLeft = true;
                    break;
                case SDLK_o:
                    crashedRight = true;
                    break;

                }
            }
            else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                case SDLK_a:
                    startGameA();
                    break;
                case SDLK_b:
                    startGameB();
                    break;
                }
            }
        }

        switch (currentMode) {
        case Mode::TIME:
            renderTime(renderer);
            break;
        case Mode::GAME_A:
        case Mode::GAME_A_HI_SCORE:
            renderGameA(renderer);
            break;
        case Mode::GAME_B:
        case Mode::GAME_B_HI_SCORE:
            renderGameB(renderer);
            break;
        default:
            break;

        }
        SDL_Delay(0);
    }
}

