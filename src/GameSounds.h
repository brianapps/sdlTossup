#ifndef GAMESOUNDS_H_
#define GAMESOUNDS_H_


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

#endif
