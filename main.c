// == TODO: make a initialize() for reset all var ==

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <libgen.h>
#include <unistd.h>

char* getResourcePath(const char* filename, bool devMode) {
    static char path[1024];
    if (devMode) {
        snprintf(path, sizeof(path), "assets/%s", filename);
    } else {
    #if defined(__APPLE__)
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        char resourcesPath[1024];
        CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)resourcesPath, sizeof(resourcesPath));
        CFRelease(resourcesURL);
        snprintf(path, sizeof(path), "%s/%s", resourcesPath, filename);
    #else
        // Linux / Windows : binaire et assets dans le même dossier
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        snprintf(path, sizeof(path), "%s/assets/%s", cwd, filename);
    #endif
    }
    return path;
}


#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define NEED_STEP 10

struct State {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} state;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect rect;
    bool visible;
} Pencil;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect rect;
    bool visible;
} Arrow;


void createWindow() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    state.window = SDL_CreateWindow("Pencil Sharpening Championship",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);

    state.renderer = SDL_CreateRenderer(state.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_RenderSetLogicalSize(state.renderer, SCREEN_WIDTH, SCREEN_HEIGHT); // keep ratio (16:9)
}

void showEndScreen(SDL_Renderer* renderer, TTF_Font* font, int score, SDL_Texture* backgroundTexture) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};

    char text[64];

    // Texte Score
    sprintf(text, "%d", score);
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, text, black);
    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_Rect scoreRect = {SCREEN_WIDTH/2 - scoreSurface->w/2 + 250, 470, scoreSurface->w, scoreSurface->h};

    bool waiting = true;
    SDL_Event event;

    while(waiting) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) exit(0);
            if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.scancode == SDL_SCANCODE_R) waiting = false; // Rejouer
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE || event.key.keysym.scancode == SDL_SCANCODE_RETURN) exit(0); // Quitter
            }
        }

        // Afficher le background
        SDL_RenderClear(renderer);
        if(backgroundTexture != NULL) {
            SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        }

        // Afficher le texte
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);

        SDL_RenderPresent(renderer);
    }

    SDL_FreeSurface(scoreSurface);
    SDL_DestroyTexture(scoreTexture);
}

Pencil spawnPencil(SDL_Renderer* renderer, bool devMode) {
    Pencil p;
    SDL_Surface* img = IMG_Load(getResourcePath("pencil.png", devMode));
    if (!img) {
        printf("Error loading pencil: %s\n", SDL_GetError());
        p.texture = NULL;
        return p;
    }

    p.texture = SDL_CreateTextureFromSurface(renderer, img);
    SDL_FreeSurface(img);

    p.rect.x = 1000;
    p.rect.y = 650;
    p.rect.w = 780;
    p.rect.h = 192;
    p.visible = true;

    return p;
}


Arrow spawnArrow(SDL_Renderer* renderer, const char* path, int x, int y, int w, int h) {
    Arrow arrow;

    SDL_Surface* img = IMG_Load(path);
    if (!img) {
        printf("Error loading arrow image (%s): %s\n", path, SDL_GetError());
        arrow.texture = NULL;
        arrow.visible = false;
        return arrow;
    }

    arrow.texture = SDL_CreateTextureFromSurface(renderer, img);
    SDL_FreeSurface(img);

    arrow.rect.x = x;
    arrow.rect.y = y;
    arrow.rect.w = w;
    arrow.rect.h = h;
    arrow.visible = true;

    return arrow;
}


int main(int argc, char * argv[]) {
    createWindow();
    srand(time( NULL )); // Random Seed

    bool devMode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-dev") == 0) {
            devMode = true;
            break;
        }
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        return -1;
    }

    bool running = true;
    SDL_Event event;

    int actualStep = 0;
    int score = 0;

    // == Background ==
    bool backgroundVisible = true;

    // == Banner ==
    SDL_Rect bannerRect;
    bannerRect.x = 0; // from left
    bannerRect.y = 50; // from top
    bannerRect.w = 768; // 512 * 1.5
    bannerRect.h = 192; // 128 * 1.5
    bool bannerVisible = true;

    // == End View ==
    bool endScreenVisible = false;

    // == Main Screen ==
    SDL_Rect mainScreenRect;
    mainScreenRect.x = 0;
    mainScreenRect.y = 0;
    mainScreenRect.w = 1920;
    mainScreenRect.h = 1080;
    bool mainScreenVisible = true;

    // == Sharpener ==
    SDL_Rect sharpenerRect;
    sharpenerRect.x = 200;
    sharpenerRect.y = 600;
    sharpenerRect.w = 504; // 420 * 1.2
    sharpenerRect.h = 307; // 256 * 1.2
    bool sharpenerVisible = true;

    Arrow leftArrow  = spawnArrow(state.renderer, getResourcePath("LeftArrow.png", devMode),  250, 447, 165, 165);
    Arrow rightArrow = spawnArrow(state.renderer, getResourcePath("RightArrow.png", devMode), 500, 440, 165, 165);
    Arrow upArrow    = spawnArrow(state.renderer, getResourcePath("UpArrow.png", devMode),    375, 440, 165, 165);



    // == Timer Text ==
    bool timerTextVisible = true;

    // == Font ==
    TTF_Font* font = TTF_OpenFont(getResourcePath("pixelFont.ttf", devMode), 96);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return -1;
    }

    SDL_Color timerTextColor = {0, 0, 0, 0}; // black
    SDL_Surface* timerTextSurface = TTF_RenderText_Solid(font, "00:00:000", timerTextColor);
    SDL_Texture* timerTextTexture = SDL_CreateTextureFromSurface(state.renderer, timerTextSurface);

    SDL_Rect timerTextRect = {150, 90, timerTextSurface->w, timerTextSurface->h};

    // === Load Images ===
    // = Background =
    SDL_Surface* backgroundIMG = IMG_Load(getResourcePath("background.png", devMode));
    if(!backgroundIMG) {
        printf("Error while loading Background Image: %s",SDL_GetError());
        return -1;
    }

    SDL_Texture* backgroundTexture = SDL_CreateTextureFromSurface(state.renderer,backgroundIMG);  // Texture -> in GPU Memory

    // = End Screen =
    SDL_Surface* endScreenIMG = IMG_Load(getResourcePath("EndView.png", devMode));
    if(!endScreenIMG) {
        printf("Error while loading Background Image: %s",SDL_GetError());
        return -1;
    }

    SDL_Texture* endScreenTexture = SDL_CreateTextureFromSurface(state.renderer,endScreenIMG);  // Texture -> in GPU Memory


    // = Banner =
    SDL_Surface* bannerIMG = IMG_Load(getResourcePath("banner_w:_clock.png", devMode));
    if(!bannerIMG) {
        printf("Error while loading Banner Image: %s",SDL_GetError());
        return -1;
    }

    SDL_Texture* bannerTexture = SDL_CreateTextureFromSurface(state.renderer,bannerIMG);

    // = Main Screen =
    SDL_Surface* mainScreenIMG = IMG_Load(getResourcePath("MainScreen.png", devMode));
    if(!mainScreenIMG) {
        printf("Error while loading Main Screen Image: %s",SDL_GetError());
        return -1;
    }

    SDL_Texture* mainScreenTexture = SDL_CreateTextureFromSurface(state.renderer,mainScreenIMG);

    // = Pencil =
    Pencil pencil = spawnPencil(state.renderer, devMode);

    actualStep = 0;


    // = Sharpener =
    SDL_Surface* sharpenerIMG = IMG_Load(getResourcePath("sharpener.png", devMode));
    if(!sharpenerIMG) {
        printf("Error while loading Sharpener Image: %s",SDL_GetError());
        return -1;
    }

    SDL_Texture* sharpenerTexture = SDL_CreateTextureFromSurface(state.renderer,sharpenerIMG);

    int randNum = rand() % 3; // arrow chooser

    Uint32 startTime = 0;
    int countdown = 11; // Countdown in sec

    while (running) {

        // === Timer ===
        if (!mainScreenVisible) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsed = (currentTime - startTime) / 1000;

            int remaining = countdown - elapsed;
            if (remaining < 0) remaining = 0;

            int minutes = remaining / 60;
            int seconds = remaining % 60;
            int ms = (currentTime - startTime) % 1000;


            // == Timer Text ==
            char timerText[32];
            sprintf(timerText, "%02d:%02d:%03d", minutes, seconds, ms); // text formating :  00:00:000

            SDL_FreeSurface(timerTextSurface);
            SDL_DestroyTexture(timerTextTexture);

            timerTextSurface = TTF_RenderText_Solid(font, timerText, timerTextColor);
            timerTextTexture = SDL_CreateTextureFromSurface(state.renderer, timerTextSurface);
            timerTextRect.w = timerTextSurface->w;
            timerTextRect.h = timerTextSurface->h;
            if (remaining == 0) {
                showEndScreen(state.renderer, font, score, endScreenTexture);

                // Réinitialiser variables pour rejouer
                actualStep = 0;
                score = 0;
                pencil = spawnPencil(state.renderer, devMode);
                randNum = rand() % 3;
                mainScreenVisible = true;
                startTime = 0;
            }

        }

        const uint8_t *keystate = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if ((randNum == 0 && !mainScreenVisible && keystate[SDL_SCANCODE_LEFT]) || (randNum == 1 && keystate[SDL_SCANCODE_UP]) || (randNum == 2 && keystate[SDL_SCANCODE_RIGHT])) {
                actualStep++;
                pencil.rect.x -= 86; // to not go after the sharpener

                randNum = rand() % 3;

                leftArrow.visible  = (randNum == 0);
                upArrow.visible    = (randNum == 1);
                rightArrow.visible = (randNum == 2);
            }
            if (keystate[SDL_SCANCODE_RETURN] && mainScreenVisible) {
                // === READY ===
                SDL_Surface* readySurface = TTF_RenderText_Solid(font, "READY...", timerTextColor);
                SDL_Texture* readyTexture = SDL_CreateTextureFromSurface(state.renderer, readySurface);
                SDL_Rect readyRect = {SCREEN_WIDTH/2 - readySurface->w/2, SCREEN_HEIGHT/2 - readySurface->h/2, readySurface->w, readySurface->h};

                SDL_RenderClear(state.renderer);
                SDL_RenderCopy(state.renderer, backgroundTexture, NULL, NULL);
                SDL_RenderCopy(state.renderer, readyTexture, NULL, &readyRect);
                SDL_RenderPresent(state.renderer);
                SDL_Delay(1000);

                // === GO! ===
                SDL_Surface* goSurface = TTF_RenderText_Solid(font, "GO!", timerTextColor);
                SDL_Texture* goTexture = SDL_CreateTextureFromSurface(state.renderer, goSurface);
                SDL_Rect goRect = {SCREEN_WIDTH/2 - goSurface->w/2, SCREEN_HEIGHT/2 - goSurface->h/2, goSurface->w, goSurface->h};

                SDL_RenderClear(state.renderer);
                SDL_RenderCopy(state.renderer, backgroundTexture, NULL, NULL);
                SDL_RenderCopy(state.renderer, goTexture, NULL, &goRect);
                SDL_RenderPresent(state.renderer);
                SDL_Delay(500);

                SDL_FreeSurface(readySurface);
                SDL_FreeSurface(goSurface);
                SDL_DestroyTexture(readyTexture);
                SDL_DestroyTexture(goTexture);

                // === Start the game ===
                mainScreenVisible = false;
                startTime = SDL_GetTicks();
            }

        }
        // === Logic ===
        if (actualStep >= NEED_STEP) {
            SDL_DestroyTexture(pencil.texture);
            pencil = spawnPencil(state.renderer, devMode);
            actualStep = 0;
            score++;
        }


        leftArrow.visible = (randNum == 0);
        upArrow.visible = (randNum == 1);
        rightArrow.visible = (randNum == 2);
        // === Render ===
        SDL_RenderClear(state.renderer);

        // = Image =
        if (backgroundVisible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, backgroundTexture, NULL, NULL);
        }

        if (bannerVisible) {
            SDL_RenderCopy(state.renderer, bannerTexture, NULL, &bannerRect);
        }

        if (endScreenVisible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, endScreenTexture, NULL, NULL);
        }

        if (pencil.visible) {
            SDL_RenderCopy(state.renderer, pencil.texture, NULL, &pencil.rect);
        }

        if (sharpenerVisible) {
            SDL_RenderCopy(state.renderer, sharpenerTexture, NULL, &sharpenerRect);
        }

        if (leftArrow.visible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, leftArrow.texture, NULL, &leftArrow.rect);
        }

        if (rightArrow.visible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, rightArrow.texture, NULL, &rightArrow.rect);
        }

        if (upArrow.visible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, upArrow.texture, NULL, &upArrow.rect);
        }

        if (mainScreenVisible) {
            SDL_RenderCopy(state.renderer, mainScreenTexture, NULL, &mainScreenRect);
        }

        // = Text =
        if (timerTextVisible && !mainScreenVisible) {
            SDL_RenderCopy(state.renderer, timerTextTexture, NULL, &timerTextRect);
        }

        // == Render (real) ==
        SDL_RenderPresent(state.renderer);
    }

    // === Destroy ===
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(bannerTexture);
    SDL_DestroyTexture(pencil.texture);
    SDL_DestroyTexture(sharpenerTexture);
    SDL_DestroyTexture(leftArrow.texture);
    SDL_DestroyTexture(rightArrow.texture);
    SDL_DestroyTexture(upArrow.texture);
    SDL_DestroyTexture(timerTextTexture);
    SDL_DestroyTexture(mainScreenTexture);
    SDL_FreeSurface(backgroundIMG);
    SDL_FreeSurface(sharpenerIMG);
    SDL_FreeSurface(bannerIMG);
    SDL_FreeSurface(mainScreenIMG);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

}
