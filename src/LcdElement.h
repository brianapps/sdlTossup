#ifndef LCDELEMENT_H_
#define LCDELEMENT_H_

#include "Outlines.h"
#include "Path.h"

class Bounds {
private:
    int xMin, xMax, yMin, yMax;

public:
    double pathUnitsPerPixel;
    double xo, yo;

    void computeBounds(int width, int height);

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
        return Point{ (node.x - xo) / pathUnitsPerPixel, (node.y - yo) / pathUnitsPerPixel };
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

    void createSurface(int outlineID, const ElementParameters& elementParameters);
    
    void createTexture(SDL_Renderer* renderer);
    
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

#endif  // LCDELEMENT_H_
