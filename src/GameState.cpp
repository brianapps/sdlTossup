#include <SDL.h>
#include <algorithm>
#include <ctime>

#include "GameState.h"
#include "LcdElement.h"
#include "GameSounds.h"

namespace {
    // see https://en.wikipedia.org/wiki/Seven-segment_display
    const uint8_t digitToSegments[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

    const uint32_t gameDelays[] = {
        393, 336, 254, 243, 231, 226, 214, 203, 192, 180, 169, 157, 146, 134,
            124, 112, 100, 89
    };
}

GameState::GameState() {
    willDropInner = false;
    willDropMid = false;
    willDropOuter = false;
    mutex = SDL_CreateMutex();
    timerID = 0;
    gameSounds.init();

}

GameState::~GameState() {
    SDL_DestroyMutex(mutex);
    if (timerID != 0)
        SDL_RemoveTimer(timerID);
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
    int hour = (localTime->tm_hour % 12);
    if (hour == 0)
        hour = 12;
    renderScore(renderer, hour * 100 + localTime->tm_min);
}


void GameState::renderGameA(SDL_Renderer* renderer) {
    renderFrameAndJuggler(renderer);
    if (outerBallPos >= 0 && outerBallPos < 12)
        textures[Outlines::OUTER0 + outerBallPos].render(renderer);
    else if (outerBallPos >= 12 && outerBallPos < 22)
        textures[Outlines::OUTER0 + 22 - outerBallPos].render(renderer);

    if (midBallPos >= 0 && midBallPos < 10)
        textures[Outlines::MID0 + midBallPos].render(renderer);
    else if (midBallPos >= 10 && midBallPos < 18)
        textures[Outlines::MID0 + 18 - midBallPos].render(renderer);

    if (innerBallPos >= 0 && innerBallPos < 8)
        textures[Outlines::INNER0 + innerBallPos].render(renderer);
    else if (innerBallPos >= 8  && innerBallPos < 14)
        textures[Outlines::INNER0 + 14 - innerBallPos].render(renderer);

    renderScore(renderer, score);
}


void GameState::renderGameB(SDL_Renderer* renderer) {
    renderGameA(renderer);
}


bool GameState::moveBall(int& currentPosition, int maxPosition, bool& willDropFlag, int catchRightPosition) {
    currentPosition = (currentPosition + 1) % maxPosition;
    if (currentPosition == maxPosition / 2) {
        willDropFlag = armPosition != (2 - catchRightPosition);
        if (!willDropFlag)
            catches++;
    }
    else if (currentPosition == 0) {
        willDropFlag = armPosition != catchRightPosition;
        if (!willDropFlag)
            catches++;
    }
    else if (gamePosition > 3)
    {
        if (currentPosition == 1 || currentPosition == (maxPosition / 2) + 1) {
            if (willDropFlag) {
                crashedRight = currentPosition == 1;
                crashedLeft = currentPosition != 1;
                currentPosition = -1;
                return false;
            }
        }
    }
    return true;
}


 
Uint32 GameState::timerCallback() {
    SDL_LockMutex(mutex);
    int ballIndex;
    if (currentMode == Mode::GAME_A)
        ballIndex = 1 + gamePosition % 2;
    else
        ballIndex = gamePosition % 3;

    bool playCatch = catches > 0;

    switch (ballIndex) {
        case 0:
            if (moveBall(innerBallPos, 14, willDropInner, 0))
                gameSounds.playInnerBeep();
            break;
        case 1:
            if (moveBall(midBallPos, 18, willDropMid, 1))
                gameSounds.playMidBeep();
            break;
        case 2:
            if (moveBall(outerBallPos, 22, willDropOuter, 2))
                gameSounds.playOuterBeep();
            break;
    }

    if (isShowingCrashed())
        gameSounds.playDropBeep();
    else if (playCatch) {
        score += currentMode == Mode::GAME_A ? 1 : 10;
        score %= 10000;
        gameSounds.playCatchBeep();
        catches--;
    }
    gamePosition++;

    Uint32 nextDelay = 0;
    if (!isShowingCrashed()) {
        if (currentMode == Mode::GAME_A)
        {
            if (score < 5)
                nextDelay = gameDelays[0];
            else if (score < 10)
                nextDelay = gameDelays[1];
            else if (score < 20)
                nextDelay = gameDelays[3];
            else {
                int hundreds = score / 100;
                int  tens = (score / 10) % 10;
                size_t index = std::min(17, 2 + tens + (hundreds >= 4 ? 12 : hundreds * 2));
                nextDelay = gameDelays[index];
            }
        }
        else {
            int thousands = score / 1000;
            int hundreds = (score / 100) % 10;
            int index = std::min(17, 2 + hundreds + (thousands >= 4 ? 12 : thousands * 2));
            nextDelay = gameDelays[index];
        }
    }
    else {
        if (currentMode == Mode::GAME_A)
            gameAHiScore = std::max(gameAHiScore, score);
        else
            gameBHiScore = std::max(gameBHiScore, score);
    }

    SDL_UnlockMutex(mutex);
    return nextDelay;
}

Uint32 GameState::staticTimerCallback(Uint32 interval, void* param) {
    return reinterpret_cast<GameState*>(param)->timerCallback();
}


void GameState::resetGameState() {
    score = 0;
    catches = 0;
    outerBallPos = 11;
    midBallPos = 0;
    innerBallPos = 7;
    gamePosition = 0;
    crashedLeft = false;
    crashedRight = false;
}


void GameState::startGameA() {
    if (!isRunningGame()) {
        resetGameState();
        currentMode = Mode::GAME_A;
        innerBallPos = -1;
        timerID = SDL_AddTimer(1, staticTimerCallback, this);

    }
}

void GameState::startGameAHiScore() {
    if (!isRunningGame()) {
        resetGameState();
        currentMode = Mode::GAME_A_HI_SCORE;
        innerBallPos = -1;
        score = gameAHiScore;
    }
}

void GameState::startGameB() {
    if (!isRunningGame()) {
        resetGameState();
        currentMode = Mode::GAME_B;
        timerID = SDL_AddTimer(1, staticTimerCallback, this);
    }
}

void GameState::startGameBHiScore() {
    if (!isRunningGame()) {
        resetGameState();
        currentMode = Mode::GAME_B_HI_SCORE;
        score = gameBHiScore;
    }
}

void GameState::startTimeMode() {
    if (isShowingCrashed()) {
        gamePosition = 0;
        crashedLeft = false;
        crashedRight = false;
        timeModeStartedTick = SDL_GetTicks();
        currentMode = Mode::TIME;
    }
}

void GameState::setArmPosition(uint32_t newArmPosition) {
    if (newArmPosition != armPosition && newArmPosition >= 0 && newArmPosition <= 2) {
        armPosition = newArmPosition;
        if (willDropMid && armPosition == 1) {
            willDropMid = false;
            catches++;
        }
        if (willDropOuter && ((armPosition == 2 && outerBallPos == 0) || (armPosition == 0 && outerBallPos == 11))) {
            willDropOuter = false;
            catches++;
        }
        if (willDropInner && ((armPosition == 0 && innerBallPos == 0) || (armPosition == 2 && innerBallPos == 7))) {
            willDropInner = false;
            catches++;
        }
    }
}

void GameState::moveArmsRight() {
    if (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B)
        setArmPosition(armPosition + 1);
}

void GameState::moveArmsLeft() {
    if (currentMode == Mode::GAME_A || currentMode == Mode::GAME_B)
        setArmPosition(armPosition - 1);
}

void GameState::run(SDL_Renderer* renderer) {
    while (true) {

        SDL_Event event;
        SDL_LockMutex(mutex);
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
        SDL_UnlockMutex(mutex);
        SDL_RenderPresent(renderer);
        SDL_Delay(0);
    }
}

