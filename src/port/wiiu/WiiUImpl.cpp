#ifdef __WIIU__
#include "WiiUImpl.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/iosupport.h>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <coreinit/debug.h>

#include <SDL2/SDL.h>

#include "Context.h"

namespace Ship {
namespace WiiU {

static bool updateControllers;
static std::map<int, SDL_GameController*> controllers;

static bool hasVpad = false;
static VPADReadError vpadError;
static VPADStatus vpadStatus;

static bool hasKpad[4] = { false };
static KPADError kpadError[4] = { KPAD_ERROR_OK };
static KPADStatus kpadStatus[4];

#ifdef _DEBUG
extern "C" {
void __wrap_abort() {
    printf("Abort called.\n");
    // force a stack trace
    *(uint32_t*)0xdeadc0de = 0xcafebabe;
    while (1)
        ;
}

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%*.*s", len, len, ptr);
    OSReport(buf);
    WHBLogWritef("%*.*s", len, len, ptr);
    return len;
}

static const devoptab_t dotab_stdout = {
    .name = "stdout_whb",
    .write_r = wiiu_log_write,
};
};
#endif

void Init(const std::string& shortName) {
#ifdef _DEBUG
    WHBLogUdpInit();
    WHBLogPrint("Hello World!");

    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;
#endif

    // make sure the required folders exist
    mkdir("/vol/external01/wiiu/", 0755);
    mkdir("/vol/external01/wiiu/apps/", 0755);
    mkdir(("/vol/external01/wiiu/apps/" + shortName + "/").c_str(), 0755);

    chdir(("/vol/external01/wiiu/apps/" + shortName + "/").c_str());

    // We construct or input based on SDL
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    updateControllers = true;
}

void Exit() {
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);

    WHBLogUdpDeinit();
}

void ThrowMissingOTR(const char* otrPath) {
    // TODO handle this better in the future
    OSFatal("Main OTR file not found!");
}

void ThrowInvalidOTR() {
    OSFatal("Invalid OTR files! Try regenerating them!");
}

static void UpdateVPADButton(VPADStatus* status, SDL_GameController* controller, VPADButtons button, SDL_GameControllerButton sdl_button)
{
    if (SDL_GameControllerGetButton(controller, sdl_button) != 0) {
        // Set the trigger bit if it wasn't held before
        if (!(status->hold & button)) {
            status->trigger |= button;
        } else {
            status->trigger &= ~button;
        }

        status->hold |= button;
        status->release &= ~button;
    } else {
        // Set the release bit if it was held before
        if (status->hold & button) {
            status->release |= button;
        } else {
            status->release &= ~button;
        }

        status->hold &= ~button;
        status->trigger &= ~button;
    }
}

static void UpdateKPADProButton(KPADStatus* status, SDL_GameController* controller, WPADProButton button, SDL_GameControllerButton sdl_button)
{
    if (SDL_GameControllerGetButton(controller, sdl_button) != 0) {
        // Set the trigger bit if it wasn't held before
        if (!(status->pro.hold & button)) {
            status->pro.trigger |= button;
        } else {
            status->pro.trigger &= ~button;
        }

        status->pro.hold |= button;
        status->pro.release &= ~button;
    } else {
        // Set the release bit if it was held before
        if (status->pro.hold & button) {
            status->pro.release |= button;
        } else {
            status->pro.release &= ~button;
        }

        status->pro.hold &= ~button;
        status->pro.trigger &= ~button;
    }
}

void Update() {
    SDL_PumpEvents();

    bool updateControllers = false;
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEREMOVED) > 0) {
        updateControllers = true;
    }

    if (updateControllers) {
        controllers.clear();
        hasVpad = false;

        int numJoysticks = SDL_NumJoysticks();
        for (int i = 0; i < numJoysticks; i++) {
            if (SDL_IsGameController(i)) {
                SDL_GameController* controller = SDL_GameControllerOpen(i);
                if (controller) {
                    int playerIndex = SDL_GameControllerGetPlayerIndex(controller);
                    if (playerIndex == 0) {
                        hasVpad = true;
                    }

                    controllers.emplace(playerIndex, controller);
                }
            }
        }
    }

    // Reconstruct VPAD/KPAD from SDL input
    // This is somewhat hacky, but we can't call VPADRead again or we steal inputs from SDL
    // for (auto& [index, controller] : controllers) {
    //     if (index == 0) {
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_A,        SDL_CONTROLLER_BUTTON_A);
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_B,        SDL_CONTROLLER_BUTTON_B);
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_X,        SDL_CONTROLLER_BUTTON_X);
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_Y,        SDL_CONTROLLER_BUTTON_Y);
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_PLUS,     SDL_CONTROLLER_BUTTON_START);
    //         UpdateVPADButton(&vpadStatus, controller, VPAD_BUTTON_MINUS,    SDL_CONTROLLER_BUTTON_BACK);
    //     } else {

    //     }
    // }

    if (hasVpad) {
        vpadStatus.tpNormal.touched = false;

        int numTouchDevices = SDL_GetNumTouchDevices();
        if (numTouchDevices > 0) {
            SDL_TouchID touchId = SDL_GetTouchDevice(0);
            int numFingers = SDL_GetNumTouchFingers(touchId);
            if (numFingers > 0) {
                SDL_Finger *finger = SDL_GetTouchFinger(touchId, 0);
                if (finger) {
                    vpadStatus.tpNormal.touched = true;
                    vpadStatus.tpNormal.validity = VPAD_VALID;
                    vpadStatus.tpNormal.x = finger->x * 1280;
                    vpadStatus.tpNormal.y = finger->y * 720;
                }
            }
        }
    }
}

VPADStatus* GetVPADStatus(VPADReadError* error) {
    *error = vpadError;
    return hasVpad ? &vpadStatus : nullptr;
}

KPADStatus* GetKPADStatus(WPADChan chan, KPADError* error) {
    *error = kpadError[chan];
    return hasKpad[chan] ? &kpadStatus[chan] : nullptr;
}

}; // namespace WiiU
}; // namespace Ship

#endif
