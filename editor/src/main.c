/*
 * A window around the save format.
 *
 * SDL3 for the window, its event loop and its file dialogs, so one program
 * runs on Windows, macOS and Linux without a platform branch; ImGui through
 * its OpenGL 3 backend, which is the pairing this repository already builds.
 */

#include "editor_ui.h"

#include "cimgui.h"
#include "cimgui_impl.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <dirent.h>
#endif

/* OpenGL 1.1, so every platform's own library has these. */
extern void glViewport(int x, int y, int width, int height);
extern void glClearColor(float r, float g, float b, float a);
extern void glClear(unsigned int mask);
extern void glReadPixels(int x, int y, int width, int height, unsigned int fmt,
                         unsigned int type, void *pixels);
#define RAGE_GL_COLOR_BUFFER_BIT 0x00004000u
#define RAGE_GL_RGB 0x1907u
#define RAGE_GL_UNSIGNED_BYTE 0x1401u

/*
 * One frame to a file. This exists so that the window can be checked without
 * a person looking at it, which is the only way an automated test can say the
 * editor draws anything at all.
 */
static int WriteFramePpm(const char *path, int width, int height) {
    unsigned char *pixels = malloc((size_t)width * height * 3);
    FILE *stream;
    int row;

    if (pixels == NULL) return 0;
    glReadPixels(0, 0, width, height, RAGE_GL_RGB, RAGE_GL_UNSIGNED_BYTE,
                 pixels);
    stream = fopen(path, "wb");
    if (stream == NULL) {
        free(pixels);
        return 0;
    }
    fprintf(stream, "P6\n%d %d\n255\n", width, height);
    /* OpenGL hands back the bottom row first. */
    for (row = height - 1; row >= 0; row--)
        fwrite(pixels + (size_t)row * width * 3, 1, (size_t)width * 3, stream);
    free(pixels);
    return fclose(stream) == 0;
}

typedef struct Shell {
    EditorState editor;
    EditorRequests requests;
    SDL_Window *window;
    int pendingOpen;
    int pendingSave;
    char chosen[1024];
} Shell;

/*
 * The bundled font is a bitmap one meant for debug overlays, and it makes an
 * ordinary program look like a terminal. A system face is used when one is
 * there, which on most desktops it is.
 *
 * ImGui does not return a failure when it cannot open a font: it asserts and
 * takes the program with it. So the file is opened here first, and only a
 * path that actually exists is handed over. Anything else and the editor
 * would die on a machine that simply has its fonts somewhere else, which is
 * every distribution that is not the one this list was written on.
 */
static int FileExists(const char *path) {
    FILE *stream = fopen(path, "rb");
    if (stream == NULL) return 0;
    fclose(stream);
    return 1;
}

#ifndef _WIN32
/*
 * Distributions put their fonts wherever they like, so when none of the
 * usual paths is there the directories themselves are searched. A name from
 * the list is preferred; failing that any face at all beats the bitmap one.
 */
static int FindFontIn(const char *directory, int depth, char *out,
                      size_t size) {
    static const char *const kPreferred[] = {
        "DejaVuSans.ttf", "NotoSans-Regular.ttf", "LiberationSans-Regular.ttf",
        "Ubuntu-R.ttf",   "Roboto-Regular.ttf",   "FreeSans.ttf",
        NULL
    };
    DIR *handle;
    struct dirent *item;
    char fallback[1024];
    int haveFallback = 0;

    if (depth > 3) return 0;
    handle = opendir(directory);
    if (handle == NULL) return 0;
    while ((item = readdir(handle)) != NULL) {
        char path[1024];
        const char *dot;
        int i;

        if (item->d_name[0] == '.') continue;
        if (snprintf(path, sizeof(path), "%s/%s", directory, item->d_name) >=
            (int)sizeof(path))
            continue;
        if (FindFontIn(path, depth + 1, out, size)) {
            closedir(handle);
            return 1;
        }
        dot = strrchr(item->d_name, '.');
        if (dot == NULL || (strcmp(dot, ".ttf") != 0 &&
                            strcmp(dot, ".otf") != 0))
            continue;
        for (i = 0; kPreferred[i] != NULL; i++) {
            if (strcmp(item->d_name, kPreferred[i]) == 0) {
                snprintf(out, size, "%s", path);
                closedir(handle);
                return 1;
            }
        }
        if (!haveFallback) {
            snprintf(fallback, sizeof(fallback), "%s", path);
            haveFallback = 1;
        }
    }
    closedir(handle);
    if (haveFallback) {
        snprintf(out, size, "%s", fallback);
        return 1;
    }
    return 0;
}
#endif

static int LoadFont(ImGuiIO *io) {
    static const char *const kCandidates[] = {
#if defined(_WIN32)
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/arial.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
#else
        /* Debian and Ubuntu */
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        /* Arch, Fedora and openSUSE */
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
        /* Alpine and a few others */
        "/usr/share/fonts/ttf-dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/noto-fonts/NotoSans-Regular.ttf",
#endif
        NULL
    };
    int i;

    for (i = 0; kCandidates[i] != NULL; i++) {
        if (!FileExists(kCandidates[i])) continue;
        if (ImFontAtlas_AddFontFromFileTTF(io->Fonts, kCandidates[i], 18.0f,
                                           NULL, NULL) != NULL)
            return 1;
    }
#ifndef _WIN32
    {
        static const char *const kRoots[] = {
            "/usr/share/fonts", "/usr/local/share/fonts",
            "/run/host/usr/share/fonts", NULL
        };
        char found[1024];
        int root;

        for (root = 0; kRoots[root] != NULL; root++) {
            if (FindFontIn(kRoots[root], 0, found, sizeof(found)) &&
                ImFontAtlas_AddFontFromFileTTF(io->Fonts, found, 18.0f, NULL,
                                               NULL) != NULL)
                return 1;
        }
    }
#endif
    /* No system face at all: the bundled one, scaled up so it is legible. */
    ImFontAtlas_AddFontDefault(io->Fonts, NULL);
    return 0;
}

/*
 * A dark room with one warm colour in it. Everything that can be pressed or
 * changed is amber; everything else is grey, so the eye finds the controls
 * without reading. Red is kept for when something is wrong.
 */
static void ApplyStyle(int haveSystemFont) {
    ImGuiStyle *style = igGetStyle();
    const ImVec4_c ink = {0.93f, 0.93f, 0.95f, 1.0f};
    const ImVec4_c amber = {0.98f, 0.63f, 0.16f, 1.0f};
    const ImVec4_c amberDim = {0.74f, 0.46f, 0.12f, 1.0f};
    const ImVec4_c panel = {0.106f, 0.106f, 0.118f, 1.0f};
    const ImVec4_c raised = {0.145f, 0.145f, 0.161f, 1.0f};
    const ImVec4_c field = {0.192f, 0.196f, 0.212f, 1.0f};
    int i;

    igStyleColorsDark(NULL);
    if (!haveSystemFont) style->FontScaleMain = 1.4f;

    style->WindowPadding = (ImVec2_c){20.0f, 18.0f};
    style->FramePadding = (ImVec2_c){11.0f, 8.0f};
    style->ItemSpacing = (ImVec2_c){12.0f, 10.0f};
    style->ItemInnerSpacing = (ImVec2_c){9.0f, 7.0f};
    style->CellPadding = (ImVec2_c){10.0f, 7.0f};
    style->IndentSpacing = 22.0f;
    style->ScrollbarSize = 13.0f;
    style->GrabMinSize = 14.0f;
    style->FrameRounding = 7.0f;
    style->GrabRounding = 7.0f;
    style->PopupRounding = 8.0f;
    style->ChildRounding = 10.0f;
    style->TabRounding = 7.0f;
    style->ScrollbarRounding = 9.0f;
    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize = 0.0f;
    style->FrameBorderSize = 0.0f;
    style->SeparatorTextBorderSize = 1.0f;
    style->SeparatorTextPadding = (ImVec2_c){0.0f, 10.0f};
    /* Selectables carry the sidebar, so their text is inset rather than
     * pressed against the edge of the strip. */
    style->SelectableTextAlign = (ImVec2_c){0.0f, 0.5f};
    style->WindowMinSize = (ImVec2_c){64.0f, 64.0f};

    for (i = 0; i < ImGuiCol_COUNT; i++) {
        ImVec4_c *colour = &style->Colors[i];
        /* Start from a single grey so nothing keeps ImGui's blue. */
        colour->x = raised.x;
        colour->y = raised.y;
        colour->z = raised.z;
    }
    style->Colors[ImGuiCol_Text] = ink;
    style->Colors[ImGuiCol_TextDisabled] = (ImVec4_c){0.55f, 0.56f, 0.60f, 1.0f};
    style->Colors[ImGuiCol_WindowBg] = panel;
    style->Colors[ImGuiCol_ChildBg] = (ImVec4_c){0.129f, 0.129f, 0.145f, 1.0f};
    style->Colors[ImGuiCol_PopupBg] = raised;
    style->Colors[ImGuiCol_Border] = (ImVec4_c){0.22f, 0.22f, 0.24f, 1.0f};
    style->Colors[ImGuiCol_FrameBg] = field;
    style->Colors[ImGuiCol_FrameBgHovered] = (ImVec4_c){0.24f, 0.24f, 0.26f, 1.0f};
    style->Colors[ImGuiCol_FrameBgActive] = (ImVec4_c){0.27f, 0.27f, 0.30f, 1.0f};
    style->Colors[ImGuiCol_TitleBg] = panel;
    style->Colors[ImGuiCol_TitleBgActive] = panel;
    style->Colors[ImGuiCol_MenuBarBg] = panel;
    style->Colors[ImGuiCol_ScrollbarBg] = panel;
    style->Colors[ImGuiCol_ScrollbarGrab] = (ImVec4_c){0.28f, 0.28f, 0.31f, 1.0f};
    style->Colors[ImGuiCol_ScrollbarGrabHovered] =
        (ImVec4_c){0.35f, 0.35f, 0.38f, 1.0f};
    style->Colors[ImGuiCol_ScrollbarGrabActive] = amberDim;
    style->Colors[ImGuiCol_CheckMark] = amber;
    style->Colors[ImGuiCol_SliderGrab] = amberDim;
    style->Colors[ImGuiCol_SliderGrabActive] = amber;
    style->Colors[ImGuiCol_Button] = (ImVec4_c){0.23f, 0.23f, 0.26f, 1.0f};
    style->Colors[ImGuiCol_ButtonHovered] = amberDim;
    style->Colors[ImGuiCol_ButtonActive] = amber;
    style->Colors[ImGuiCol_Header] = (ImVec4_c){0.26f, 0.20f, 0.10f, 1.0f};
    style->Colors[ImGuiCol_HeaderHovered] = (ImVec4_c){0.32f, 0.25f, 0.12f, 1.0f};
    style->Colors[ImGuiCol_HeaderActive] = amberDim;
    style->Colors[ImGuiCol_Separator] = (ImVec4_c){0.24f, 0.24f, 0.27f, 1.0f};
    style->Colors[ImGuiCol_Tab] = (ImVec4_c){0.17f, 0.17f, 0.19f, 1.0f};
    style->Colors[ImGuiCol_TabHovered] = amberDim;
    style->Colors[ImGuiCol_TabSelected] = (ImVec4_c){0.26f, 0.20f, 0.10f, 1.0f};
    style->Colors[ImGuiCol_TabDimmed] = (ImVec4_c){0.15f, 0.15f, 0.17f, 1.0f};
    style->Colors[ImGuiCol_TabDimmedSelected] =
        (ImVec4_c){0.22f, 0.18f, 0.10f, 1.0f};
    style->Colors[ImGuiCol_TableHeaderBg] = (ImVec4_c){0.17f, 0.17f, 0.19f, 1.0f};
    style->Colors[ImGuiCol_TableBorderStrong] =
        (ImVec4_c){0.24f, 0.24f, 0.27f, 1.0f};
    style->Colors[ImGuiCol_TableBorderLight] =
        (ImVec4_c){0.19f, 0.19f, 0.21f, 1.0f};
    style->Colors[ImGuiCol_TableRowBg] = (ImVec4_c){0.0f, 0.0f, 0.0f, 0.0f};
    style->Colors[ImGuiCol_TableRowBgAlt] = (ImVec4_c){1.0f, 1.0f, 1.0f, 0.022f};
    style->Colors[ImGuiCol_NavCursor] = amber;
}

static const SDL_DialogFileFilter kFilters[] = {
    {"Rage Racer save", "*"},
};

static void ChoseFile(void *userdata, const char *const *files, int filter) {
    Shell *shell = userdata;

    (void)filter;
    /* A cancelled dialog hands back an empty list rather than an error. */
    if (files == NULL || files[0] == NULL) {
        shell->pendingOpen = 0;
        shell->pendingSave = 0;
        return;
    }
    snprintf(shell->chosen, sizeof(shell->chosen), "%s", files[0]);
}

/* Asked for by a button in the editor, opened here. */
static void ServeRequests(Shell *shell) {
    if (shell->editor.dirty || 1) { /* nothing to confirm yet */ }
    if (shell->requests.open) {
        shell->requests.open = 0;
        shell->pendingOpen = 1;
        shell->chosen[0] = '\0';
        SDL_ShowOpenFileDialog(ChoseFile, shell, shell->window, kFilters, 1,
                               NULL, false);
    }
    if (shell->requests.saveAs) {
        char directory[1024];
        char suggestion[1200];
        char name[64];

        shell->requests.saveAs = 0;
        shell->pendingSave = 1;
        shell->chosen[0] = '\0';
        EditorSuggestedName(&shell->editor, name, sizeof(name));
        /* Offer the card directory and the name the game expects. */
        if (RageSaveCardDirectory(0, directory, sizeof(directory)))
            snprintf(suggestion, sizeof(suggestion), "%s/%s", directory, name);
        else
            snprintf(suggestion, sizeof(suggestion), "%s", name);
        SDL_ShowSaveFileDialog(ChoseFile, shell, shell->window, kFilters, 1,
                               suggestion);
    }
}

/* The dialogs answer on their own thread on some platforms, so the answer is
 * picked up here rather than acted on inside the callback. */
static void CollectDialogAnswer(Shell *shell) {
    if (shell->chosen[0] == '\0') return;
    if (shell->pendingOpen) {
        EditorOpen(&shell->editor, shell->chosen);
        shell->pendingOpen = 0;
    } else if (shell->pendingSave) {
        EditorSave(&shell->editor, shell->chosen);
        shell->pendingSave = 0;
    }
    shell->chosen[0] = '\0';
}

int main(int argc, char **argv) {
    Shell shell;
    SDL_GLContext context;
    ImGuiIO *io;
    int running = 1;
    const char *screenshot = NULL;
    int frames = 0;

    memset(&shell, 0, sizeof(shell));
    shell.editor.region = RAGE_REGION_PAL;
    shell.editor.logoZoom = 6;
    snprintf(shell.editor.status, sizeof(shell.editor.status),
             "open a save, or start a new one");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL could not start: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    shell.window = SDL_CreateWindow("Rage Racer save editor", 1200, 800,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                        SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (shell.window == NULL) {
        fprintf(stderr, "no window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    context = SDL_GL_CreateContext(shell.window);
    if (context == NULL) {
        fprintf(stderr, "no OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(shell.window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(shell.window, context);
    SDL_GL_SetSwapInterval(1);

    igCreateContext(NULL);
    io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->IniFilename = NULL;
    ApplyStyle(LoadFont(io));
    ImGui_ImplSDL3_InitForOpenGL(shell.window, context);
    ImGui_ImplOpenGL3_Init("#version 150");

    {
        int arg;
        for (arg = 1; arg < argc; arg++) {
            if (strcmp(argv[arg], "--screenshot") == 0 && arg + 1 < argc)
                screenshot = argv[++arg];
            else if (strcmp(argv[arg], "--tab") == 0 && arg + 1 < argc)
                shell.editor.openTab = argv[++arg];
            else
                EditorOpen(&shell.editor, argv[arg]);
        }
    }

    while (running) {
        SDL_Event event;
        int width;
        int height;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = 0;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(shell.window))
                running = 0;
            /* Dropping a file on the window opens it. */
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != NULL)
                EditorOpen(&shell.editor, event.drop.data);
        }
        ServeRequests(&shell);
        CollectDialogAnswer(&shell);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        {
            ImGuiViewport *viewport = igGetMainViewport();
            igSetNextWindowPos(viewport->WorkPos, ImGuiCond_Always,
                               (ImVec2_c){0.0f, 0.0f});
            igSetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
            if (igBegin("Rage Racer save editor", NULL,
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoBringToFrontOnFocus)) {
                EditorDrawWindow(&shell.editor, &shell.requests);
            }
            igEnd();
        }

        igRender();
        SDL_GetWindowSizeInPixels(shell.window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.09f, 0.09f, 0.11f, 1.0f);
        glClear(RAGE_GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        /* A few frames first, so the layout has settled before it is read. */
        if (screenshot != NULL && ++frames == 3) {
            running = WriteFramePpm(screenshot, width, height) ? 0 : 0;
            if (running == 0)
                printf("wrote %s\n", screenshot);
        }
        SDL_GL_SwapWindow(shell.window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(shell.window);
    SDL_Quit();
    return 0;
}
