#include "input.h"

Input::Input() :
    held_keys_(),
    pressed_keys_(),
    released_keys_()
{}

void Input::beginNewFrame()
{
    pressed_keys_.clear();
    released_keys_.clear();
}

void Input::keyDownEvent(const SDL_Event& event)
{
    pressed_keys_[event.jbutton.button] = true;
    held_keys_[event.jbutton.button] = true;
}

void Input::keyUpEvent(const SDL_Event& event)
{
    released_keys_[event.jbutton.button] = true;
    held_keys_[event.jbutton.button] = false;
}

bool Input::wasKeyPressed(int key)
{
    return pressed_keys_[key];
}

bool Input::wasKeyReleased(int key)
{
    return released_keys_[key];
}

bool Input::isKeyHeld(int key)
{
    return held_keys_[key];
}
