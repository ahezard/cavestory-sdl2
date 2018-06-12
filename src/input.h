#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <map>

struct Input {
    Input();

    void beginNewFrame();

    void keyDownEvent(const SDL_Event& event);
    void keyUpEvent(const SDL_Event& event);

    bool wasKeyPressed(int key);
    bool wasKeyReleased(int key);
    bool isKeyHeld(int key);
private:
    std::map<int, bool> held_keys_;
    std::map<int, bool> pressed_keys_;
    std::map<int, bool> released_keys_;
};

#endif /* INPUT_H */
