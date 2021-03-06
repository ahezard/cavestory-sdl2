#include "player.h"

#include "damage_texts.h"
#include "first_cave_bat.h"
#include "game.h"
#include "input.h"
#include "map.h"
#include "particle_tools.h"
#include "projectile.h"
#include "timer.h"

const units::FPS kFps{60};
const auto kMaxFrameTime = std::chrono::milliseconds{5 * 1000 / 60};
units::Tile Game::kScreenWidth{20};
units::Tile Game::kScreenHeight{15};

#define BUTTON_DPAD_UP 13
#define BUTTON_DPAD_DOWN 15
#define BUTTON_DPAD_LEFT 12
#define BUTTON_DPAD_RIGHT 14
#define BUTTON_PLUS 10
#define BUTTON_A 0
#define BUTTON_B 1
#define BUTTON_X 2
#define BUTTON_Y 3

Game::Game() :
    sdlEngine_(),
    graphics_(),
    player_{std::make_shared<Player>(graphics_,
            Vector<units::Game>{
            units::tileToGame(Game::kScreenWidth/2),
            units::tileToGame(Game::kScreenHeight/2)}
            )
    },
    bat_{std::make_shared<FirstCaveBat>(graphics_,
            Vector<units::Game>{
            units::tileToGame(7),
            units::tileToGame(Game::kScreenHeight/2 + 1)}
            )
    },
    map_{Map::createTestMap(graphics_)},
    particle_system_{},
    damage_texts_()
{
    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when connected, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            printf("SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
        }
    }
    
    runEventLoop();
}

Game::~Game()
{
}

void Game::runEventLoop() {
    Input input;
    SDL_Event event;

    damage_texts_.addDamageable(player_);
    damage_texts_.addDamageable(bat_);

    bool running{true};
    auto last_updated_time = std::chrono::high_resolution_clock::now();
    while (running) {
        using std::chrono::high_resolution_clock;
        using std::chrono::milliseconds;
        using std::chrono::duration_cast;

        const auto start_time = high_resolution_clock::now();

        input.beginNewFrame();
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_JOYBUTTONDOWN:
                input.keyDownEvent(event);
                break;
            case SDL_JOYBUTTONUP:
                input.keyUpEvent(event);
                break;
            case SDL_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }
        if (input.wasKeyPressed(BUTTON_PLUS)) {
            running = false;
        }

        // Player Horizontal Movement
        if (input.isKeyHeld(BUTTON_DPAD_LEFT)
                && input.isKeyHeld(BUTTON_DPAD_RIGHT)) {
            // if both left and right are being pressed we need to stop moving
            player_->stopMoving();
        } else if (input.isKeyHeld(BUTTON_DPAD_LEFT)) {
            player_->startMovingLeft();
        } else if (input.isKeyHeld(BUTTON_DPAD_RIGHT)) {
            player_->startMovingRight();
        } else {
            player_->stopMoving();
        }

        if (input.isKeyHeld(BUTTON_DPAD_UP) &&
                input.isKeyHeld(BUTTON_DPAD_DOWN)) {
            player_->lookHorizontal();
        } else if (input.isKeyHeld(BUTTON_DPAD_UP)) {
            player_->lookUp();
        } else if (input.isKeyHeld(BUTTON_DPAD_DOWN)) {
            player_->lookDown();
        } else {
            player_->lookHorizontal();
        }

        // Player Jump
        if (input.wasKeyPressed(BUTTON_A)) {
            player_->startJump();
        } else if (input.wasKeyReleased(BUTTON_A)) {
            player_->stopJump();
        }
        // Player Fire
        if (input.wasKeyPressed(BUTTON_B)) {
            player_->startFire();
        } else if (input.wasKeyReleased(BUTTON_B)) {
            player_->stopFire();
        }

        // update scene and last_updated_time
        const auto current_time = high_resolution_clock::now();
        const auto upd_elapsed_time = current_time - last_updated_time;

        update(
                    std::chrono::milliseconds{ 1000 / 60},
               graphics_
              );
        last_updated_time = current_time;

        draw(graphics_);

        // calculate delay for constant fps
        const auto end_time = high_resolution_clock::now();
        const auto elapsed_time = duration_cast<milliseconds>(
                end_time - start_time);

        const auto delay_duration = milliseconds(1000) / kFps - elapsed_time;
        if (delay_duration.count() >= 0)
            SDL_Delay(delay_duration.count());
    }
}

void Game::update(const std::chrono::milliseconds elapsed_time, Graphics& graphics)
{
    Timer::updateAll(elapsed_time);
    damage_texts_.update(elapsed_time);
    particle_system_.update(elapsed_time);

    auto particle_tools = ParticleTools{ particle_system_, graphics };
    //TODO: update map when it is changed
    player_->update(elapsed_time, *map_, particle_tools);
    auto player_pos = player_->getCenterPos();
    if (bat_) {
        if (!bat_->update(elapsed_time, player_pos.x)) {
            bat_.reset();
        }
    }

    auto projectiles = player_->getProjectiles();
    for (auto projectile: projectiles) {
        auto projectile_rect = projectile->getCollisionRectangle();
        if (bat_ && bat_->getCollisionRectangle().collidesWith(projectile_rect)) {
            projectile->collideWithEnemy();
            bat_->takeDamage(projectile->getContactDamage());
        }
    }

    if (bat_) {
        const auto batRect = bat_->getDamageRectangle();
        if (batRect.collidesWith(player_->getDamageRectangle())) {
            player_->takeDamage(bat_->contactDamage());
        }
    }
}

void Game::draw(Graphics& graphics) const
{
    graphics.clear();

    map_->drawBackground(graphics);
    if (bat_)
        bat_->draw(graphics);
    player_->draw(graphics);
    map_->draw(graphics);
    particle_system_.draw(graphics);

    damage_texts_.draw(graphics);
    player_->drawHUD(graphics);

    graphics.flip();
}
