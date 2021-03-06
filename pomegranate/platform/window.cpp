#include "pch.hpp"

#include "platform/inputEvent.hpp"
#include "window.hpp"

namespace pom {
    Window::Window(const char* title, gfx::GraphicsAPI api, const maths::ivec2& position, const maths::ivec2& size) :
        graphicsAPI(api)
    {
        constexpr int DEFAULT_WIDTH = 720;
        constexpr int DEFAULT_HEIGHT = 480;

        // NOTE: maybe make this an option?
        SDL_SetHint(SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1");

        // FIXME: this code provides continuous events to both window resize and move, which SDL2 does not do by
        // default. Make sure this is safe.
        SDL_AddEventWatch(
            [](void* /*userdata*/, SDL_Event* e) -> int {
                if (e->type == SDL_WINDOWEVENT) {
                    auto* self = static_cast<Window*>(
                        SDL_GetWindowData(SDL_GetWindowFromID(e->window.windowID), POM_SDL_WINDOW_PTR));

                    switch (e->window.event) {
                    case SDL_WINDOWEVENT_MOVED: {
                        self->callbackFn({ .type = InputEventType::WINDOW_MOVE,
                                           .sourceWindow = self,
                                           .windowMoveData = { e->window.data1, e->window.data2 } });

                    } break;
                    case SDL_WINDOWEVENT_RESIZED: {
                        self->callbackFn({ .type = InputEventType::WINDOW_RESIZE,
                                           .sourceWindow = self,
                                           .windowResizeData = { e->window.data1, e->window.data2 } });

                    } break;
                    }
                }

                return 0;
            },
            nullptr);

        // TODO: load previous size/position values from file.

        u32 flags = SDL_WINDOW_RESIZABLE;
        if (graphicsAPI == gfx::GraphicsAPI::VULKAN) {
            flags |= SDL_WINDOW_VULKAN;
        }

        if (windowHandle = SDL_CreateWindow(title,
                                            position.x < 0 ? (int)SDL_WINDOWPOS_UNDEFINED : position.x,
                                            position.y < 0 ? (int)SDL_WINDOWPOS_UNDEFINED : position.y,
                                            size.x < 0 ? DEFAULT_WIDTH : size.x,
                                            size.y < 0 ? DEFAULT_HEIGHT : size.y,
                                            flags);
            !windowHandle) {
            POM_LOG_FATAL("Unable to initialize Window. error: ", SDL_GetError());
        }

        SDL_SetWindowData(windowHandle, POM_SDL_WINDOW_PTR, (void*)this);
    }

    Window::~Window()
    {
        if (windowHandle) {
            SDL_DestroyWindow(windowHandle);
        }
    }

    void Window::pollEvents()
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_WINDOWEVENT: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));
                switch (e.window.event) {
                case SDL_WINDOWEVENT_CLOSE: {
                    self->closeRequested = true;
                    self->callbackFn(
                        { .type = InputEventType::WINDOW_CLOSE, .sourceWindow = self, .windowCloseData = {} });
                } break;

                    // See above, this preserves default behavior
                    // case SDL_WINDOWEVENT_MOVED: {
                    //     self->callbackFn({ .type = InputEventType::WINDOW_MOVE,
                    //                        .sourceWindow = self,
                    //                        .windowMoveData = { e.window.data1, e.window.data2 } });

                    // } break;
                    // case SDL_WINDOWEVENT_RESIZED: {
                    //     self->callbackFn({ .type = InputEventType::WINDOW_RESIZE,
                    //                        .sourceWindow = self,
                    //                        .windowResizeData = { e.window.data1, e.window.data2 } });

                    // } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED: {
                    self->callbackFn(
                        { .type = InputEventType::WINDOW_FOCUS, .sourceWindow = self, .windowFocusData = {} });
                } break;
                case SDL_WINDOWEVENT_FOCUS_LOST: {
                    self->callbackFn(
                        { .type = InputEventType::WINDOW_BLUR, .sourceWindow = self, .windowBlurData = {} });

                } break;
                case SDL_WINDOWEVENT_MINIMIZED: {
                    self->callbackFn(
                        { .type = InputEventType::WINDOW_MINIMIZE, .sourceWindow = self, .windowMinimizeData = {} });

                } break;
                case SDL_WINDOWEVENT_MAXIMIZED: {
                    self->callbackFn(
                        { .type = InputEventType::WINDOW_MAXIMIZE, .sourceWindow = self, .windowMaximizeData = {} });

                } break;
                }
            } break;
            case SDL_MOUSEMOTION: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));

                self->callbackFn({ .type = InputEventType::MOUSE_MOVE,
                                   .sourceWindow = self,
                                   .mouseMoveData = { e.motion.x, e.motion.y } });
            } break;
            case SDL_MOUSEBUTTONDOWN: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));

                self->callbackFn({ .type = InputEventType::MOUSE_DOWN,
                                   .sourceWindow = self,
                                   .mouseDownData = { static_cast<MouseButton>(e.button.button), e.button.clicks } });
            } break;
            case SDL_MOUSEBUTTONUP: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));

                self->callbackFn({ .type = InputEventType::MOUSE_UP,
                                   .sourceWindow = self,
                                   .mouseUpData = { static_cast<MouseButton>(e.button.button), e.button.clicks } });
            } break;
            case SDL_MOUSEWHEEL: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));

                self->callbackFn({ .type = InputEventType::MOUSE_SCROLL,
                                   .sourceWindow = self,
                                   .mouseScrollData = { e.wheel.x, e.wheel.y } });
            } break;
            case SDL_KEYDOWN: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));
                self->callbackFn({ .type = InputEventType::KEY_DOWN,
                                   .sourceWindow = self,
                                   .keyDownData = { static_cast<KeyHid>(e.key.keysym.scancode),
                                                    static_cast<Keycode>(e.key.keysym.sym),
                                                    e.key.repeat > 0 } });
            } break;
            case SDL_KEYUP: {
                auto* self = static_cast<Window*>(
                    SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), POM_SDL_WINDOW_PTR));
                self->callbackFn({ .type = InputEventType::KEY_UP,
                                   .sourceWindow = self,
                                   .keyUpData = { static_cast<KeyHid>(e.key.keysym.scancode),
                                                  static_cast<Keycode>(e.key.keysym.sym),
                                                  e.key.repeat > 0 } });
            } break;
            };
        }
    }

    std::vector<const char*> Window::getRequiredVulkanExtensions() const
    {
        if (graphicsAPI != gfx::GraphicsAPI::VULKAN) {
            POM_LOG_ERROR("Attempting to get vulkan instance extensions from a non-vulkan window.");
        }

        unsigned int extensionCount = 0;

        if (!SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, nullptr)) {
            POM_LOG_FATAL("Failed to get SDL required vulkan extensions, ", SDL_GetError());
        }

        std::vector<const char*> extensions(extensionCount);

        if (!SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, extensions.data())) {
            POM_LOG_FATAL("Failed to get SDL required vulkan extensions, ", SDL_GetError());
        }

        return extensions;
    }

    VkSurfaceKHR Window::getVulkanSurface(VkInstance instance) const
    {
        if (graphicsAPI != gfx::GraphicsAPI::VULKAN) {
            POM_LOG_ERROR("Attempting to get vulkan instance extensions from a non-vulkan window.");
        }

        VkSurfaceKHR surface;

        if (!SDL_Vulkan_CreateSurface(windowHandle, instance, &surface)) {
            POM_LOG_FATAL(SDL_GetError());
        }

        return surface;
    }

    VkExtent2D Window::getVulkanDrawableExtent() const
    {
        if (graphicsAPI != gfx::GraphicsAPI::VULKAN) {
            POM_LOG_ERROR("Attempting to get vulkan drawable extent from a non-vulkan window.");
        }

        VkExtent2D extent;

        SDL_Vulkan_GetDrawableSize(windowHandle, (int*)&extent.width, (int*)&extent.height);
        return extent;
    }

} // namespace pom