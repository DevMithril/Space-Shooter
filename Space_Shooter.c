#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

/* resolution : 320x240 */
/* ligne de commande pour la compilation : gcc -o Space_Shooter Space_Shooter.c -lm $(sdl2-config --cflags --libs) -l SDL2_ttf */
/* pour utiliser valgrind (memoire) : valgrind -s --tool=memcheck --leak-check=yes|no|full|summary --leak-resolution=low|med|high --show-reachable=yes ./Space_Shooter */

typedef struct Hitbox
{
    int nb_points;
    SDL_Point *points;
    int cercle_x, cercle_y, cercle_rayon;
}Hitbox;

typedef struct Text
{
    int x, y;
    SDL_Texture *texture;
    SDL_Rect src_rect, dst_rect;
    struct Text *suivant;
}Text;

typedef struct Fonts
{
    TTF_Font *titles, *menu_button, *secondary_titles;
    SDL_Color rouge, vert, vert_clair;
}Fonts;

typedef struct Input
{
    SDL_bool key[SDL_NUM_SCANCODES];
    SDL_bool quit;
    SDL_KeyCode wanted_input;
    SDL_Keycode key_up, key_down, key_left, key_right, key_L, key_R, key_start, key_select, key_A, key_B;  /* /;8;7;9;A;Z;Return;E;Space;D */
    SDL_bool up, down, left, right, L, R, start, select, A, B;
    SDL_bool select_on_cooldown, start_on_cooldown, waiting_for_input;
}Input;

typedef struct Button
{
    Text *text;
    struct Button *upward, *downward, *to_the_left, *to_the_right;
}Button;

typedef struct Mob
{
    int x, y;
    SDL_Texture *texture;
    SDL_Rect src_rect, dst_rect;
    Hitbox hitbox;
    int PV;
    struct Mob *suivant;
}Mob;

typedef struct FirePlayer
{
    int x, y;
    SDL_Texture *texture;
    SDL_Rect src_rect, dst_rect;
    Hitbox hitbox;
    struct FirePlayer *suivant;
}FirePlayer;

typedef struct Player
{
    int x, y;
    SDL_Texture *texture;
    SDL_Rect src_rect, dst_rect;
    Hitbox hitbox;
    int PV;
    int delay_fire, invincibility_frames;
    SDL_bool fire_on_cooldown, invicible;
}Player;

typedef struct Level
{
    int x, y;
    SDL_Texture *texture;
    Text *game_over, *title, *settings_title, *control_settings_title;
    Button start_game, quit_game, settings, back_to_menu, control_settings, control_back_to_settings;
    Button chg_up, chg_down, chg_left, chg_right, chg_L, chg_R, chg_start, chg_select, chg_A, chg_B;
    Button *selected_button;
    SDL_Rect src_rect;
    int frame, delay_button;
    int nb_hitboxes;
    Hitbox *hitboxes;
}Level;

typedef struct Everything
{
    Player player;
    FirePlayer liste_fireplayer;
    Mob liste_mob;
    Level level;
    Text liste_text;
    Fonts fonts;
    int game_state;
    Input input;
    SDL_Renderer *renderer;
    SDL_Window *window;
}Everything;

void updateEvent(Input *input)
{
    SDL_Event event;
    input->wanted_input = -1;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_QUIT)
            input->quit = SDL_TRUE;
        else if(event.type == SDL_KEYDOWN)
        {
            if (input->waiting_for_input)
            {
                input->wanted_input = SDL_GetKeyFromScancode(event.key.keysym.scancode);
            }
            input->key[event.key.keysym.scancode] = SDL_TRUE;
        }
        else if(event.type == SDL_KEYUP)
            input->key[event.key.keysym.scancode] = SDL_FALSE;
    }
    input->A = input->key[SDL_GetScancodeFromKey(input->key_A)];
    input->B = input->key[SDL_GetScancodeFromKey(input->key_B)];
    input->down = input->key[SDL_GetScancodeFromKey(input->key_down)];
    input->L = input->key[SDL_GetScancodeFromKey(input->key_L)];
    input->left = input->key[SDL_GetScancodeFromKey(input->key_left)];
    input->R = input->key[SDL_GetScancodeFromKey(input->key_R)];
    input->right = input->key[SDL_GetScancodeFromKey(input->key_right)];
    input->up = input->key[SDL_GetScancodeFromKey(input->key_up)];
    if (input->select)
    {
        if (input->key[SDL_GetScancodeFromKey(input->key_select)])
        {
            input->select_on_cooldown = SDL_TRUE;
        }
        input->select = SDL_FALSE;
    }
    else if (input->select_on_cooldown)
    {
        if (!input->key[SDL_GetScancodeFromKey(input->key_select)])
        {
            input->select_on_cooldown = SDL_FALSE;
        }
    }
    else
    {
        input->select = input->key[SDL_GetScancodeFromKey(input->key_select)];
    }

    if (input->start)
    {
        if (input->key[SDL_GetScancodeFromKey(input->key_start)])
        {
            input->start_on_cooldown = SDL_TRUE;
        }
        input->start = SDL_FALSE;
    }
    else if (input->start_on_cooldown)
    {
        if (!input->key[SDL_GetScancodeFromKey(input->key_start)])
        {
            input->start_on_cooldown = SDL_FALSE;
        }
    }
    else
    {
        input->start = input->key[SDL_GetScancodeFromKey(input->key_start)];
    }
}

SDL_Texture *loadImage(const char chemin[], SDL_Renderer *renderer)
{
    SDL_Surface *surface = NULL; 
    SDL_Texture *texture = NULL, *tmp = NULL;
    surface = SDL_LoadBMP(chemin);
    if(NULL == surface)
    {
        fprintf(stderr, "Erreur SDL_LoadBMP : %s\n", SDL_GetError());
        return NULL;
    }
    tmp = SDL_CreateTextureFromSurface(renderer, surface);
    if(NULL == tmp)
    {
        fprintf(stderr, "Erreur SDL_CreateTextureFromSurface : %s\n", SDL_GetError());
        return NULL;
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                            SDL_TEXTUREACCESS_TARGET, surface->w, surface->h);
    if(NULL == texture)
    {
        fprintf(stderr, "Erreur SDL_CreateTexture : %s\n", SDL_GetError());
        return NULL;
    }
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderCopy(renderer, tmp, NULL, NULL);
    SDL_DestroyTexture(tmp);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

Text *loadText(TTF_Font *font, const char text[], SDL_Color color, Everything *all)
{
    SDL_Surface *surface = NULL; 
    SDL_Texture *texture = NULL, *tmp = NULL;
    Text *last = &all->liste_text;
    while (last->suivant != NULL)
    {
        last = last->suivant;
    }
    surface = TTF_RenderText_Solid(font, text, color);
    if(NULL == surface)
    {
        fprintf(stderr, "Erreur TTF_RenderText_Solid : %s\n", TTF_GetError());
        return NULL;
    }
    tmp = SDL_CreateTextureFromSurface(all->renderer, surface);
    if(NULL == tmp)
    {
        fprintf(stderr, "Erreur SDL_CreateTextureFromSurface : %s\n", SDL_GetError());
        return NULL;
    }
    texture = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, 
                            SDL_TEXTUREACCESS_TARGET, surface->w, surface->h);
    if(NULL == texture)
    {
        fprintf(stderr, "Erreur SDL_CreateTexture : %s\n", SDL_GetError());
        return NULL;
    }

    last->suivant = SDL_malloc(sizeof(Text));
    last->suivant->texture = texture;
    last->suivant->src_rect.h = surface->h;
    last->suivant->src_rect.w = surface->w;
    last->suivant->src_rect.x = 0;
    last->suivant->src_rect.y = 0;
    last->suivant->dst_rect.h = surface->h;
    last->suivant->dst_rect.w = surface->w;
    last->suivant->dst_rect.x = 0;
    last->suivant->dst_rect.y = 0;
    last->suivant->x = 0;
    last->suivant->y = 0;
    last->suivant->suivant = NULL;

    SDL_SetRenderTarget(all->renderer, texture);
    SDL_RenderCopy(all->renderer, tmp, NULL, NULL);
    SDL_DestroyTexture(tmp);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(all->renderer, NULL);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return last->suivant;
}

SDL_bool sat(Hitbox *hitbox1, Hitbox *hitbox2)
{
    double min1 = 0, max1 = 0, min2 = 0, max2 = 0, p = 0;
    int nb_axes1, nb_axes2;
    SDL_Point normal = {.x = 0, .y = 0};

    if ( (hitbox1->cercle_x-hitbox2->cercle_x)*(hitbox1->cercle_x-hitbox2->cercle_x) + (hitbox1->cercle_y-hitbox2->cercle_y)*(hitbox1->cercle_y-hitbox2->cercle_y) >
                     (hitbox1->cercle_rayon + hitbox2->cercle_rayon)*(hitbox1->cercle_rayon + hitbox2->cercle_rayon))
    {
        return SDL_FALSE;
    }

    for (int i = 0; i < hitbox1->nb_points; i++ )
    {
        if (i+1 >= hitbox1->nb_points)
        {
            normal.x = -(hitbox1->points[i].y - hitbox1->points[0].y);
            normal.y = hitbox1->points[i].x - hitbox1->points[0].x;
        }
        else
        {
            normal.x = -(hitbox1->points[i].y - hitbox1->points[i+1].y);
            normal.y = hitbox1->points[i].x - hitbox1->points[i+1].x;
        }
        min1 = normal.x * hitbox1->points[0].x + normal.y * hitbox1->points[0].y;
        max1 = min1;
        for (int j = 1; j < hitbox1->nb_points; j++ )
        {
            p = normal.x * hitbox1->points[j].x + normal.y * hitbox1->points[j].y;
            if (p < min1)
            {
                min1 = p;
            }
            else if (p > max1)
            {
                max1 = p;
            }
        }
        min2 = normal.x * hitbox2->points[0].x + normal.y * hitbox2->points[0].y;
        max2 = min2;
        for (int j = 1; j < hitbox2->nb_points; j++ )
        {
            p = normal.x * hitbox2->points[j].x + normal.y * hitbox2->points[j].y;
            if (p < min2)
            {
                min2 = p;
            }
            else if (p > max2)
            {
                max2 = p;
            }
        }
        if (min1 >= max2 || min2 >= max1)
        {
            return SDL_FALSE;
        }
    }
    

    for (int i = 0; i < hitbox2->nb_points; i++ )
    {
        if (i+1 >= hitbox2->nb_points)
        {
            normal.x = -(hitbox2->points[i].y - hitbox2->points[0].y);
            normal.y = hitbox2->points[i].x - hitbox2->points[0].x;
        }
        else
        {
            normal.x = -(hitbox2->points[i].y - hitbox2->points[i+1].y);
            normal.y = hitbox2->points[i].x - hitbox2->points[i+1].x;
        }
        min1 = normal.x * hitbox1->points[0].x + normal.y * hitbox1->points[0].y;
        max1 = min1;
        for (int j = 1; j < hitbox1->nb_points; j++ )
        {
            p = normal.x * hitbox1->points[j].x + normal.y * hitbox1->points[j].y;
            if (p < min1)
            {
                min1 = p;
            }
            else if (p > max1)
            {
                max1 = p;
            }
        }
        min2 = normal.x * hitbox2->points[0].x + normal.y * hitbox2->points[0].y;
        max2 = min2;
        for (int j = 1; j < hitbox2->nb_points; j++ )
        {
            p = normal.x * hitbox2->points[j].x + normal.y * hitbox2->points[j].y;
            if (p < min2)
            {
                min2 = p;
            }
            else if (p > max2)
            {
                max2 = p;
            }
        }
        if (min1 >= max2 || min2 >= max1)
        {
            return SDL_FALSE;
        }
    }

    return SDL_TRUE;
}

void destroyText(Text *text, Everything *all)
{
    Text *text_liste = &all->liste_text, *tmp = NULL;
    if (text == NULL)
    {
        fprintf(stderr, "Erreur dans destroyText : le Text passé en paramètre est un pointeur NULL");
        return;
    }
    while (text_liste->suivant != text)
    {
        text_liste = text_liste->suivant;
    }
    if (text_liste->suivant->texture != NULL)
    {
        SDL_DestroyTexture(text_liste->suivant->texture);
    }
    tmp = text_liste->suivant->suivant;
    SDL_free(text_liste->suivant);
    text_liste->suivant = tmp;
}

void destroyFonts(Everything *all)
{
    Fonts *fonts = &all->fonts;
    if (fonts->titles != NULL)
    {
        TTF_CloseFont(fonts->titles);
    }
    if (fonts->menu_button != NULL)
    {
        TTF_CloseFont(fonts->menu_button);
    }
    if (fonts->secondary_titles != NULL)
    {
        TTF_CloseFont(fonts->secondary_titles);
    }
}

void loadFonts(Everything *all)
{
    Fonts *fonts = &all->fonts;
    fonts->titles = TTF_OpenFont("data/8-bitanco.ttf", 30);
    if (fonts->titles == NULL)
    {
        fprintf(stderr, "Erreur TTF_OpenFont : %s\n", TTF_GetError());
    }
    fonts->menu_button = TTF_OpenFont("data/alagard.ttf", 18);
    if (fonts->menu_button == NULL)
    {
        fprintf(stderr, "Erreur TTF_OpenFont : %s\n", TTF_GetError());
    }
    fonts->secondary_titles = TTF_OpenFont("data/upheavtt.ttf", 30);
    if (fonts->secondary_titles == NULL)
    {
        fprintf(stderr, "Erreur TTF_OpenFont : %s\n", TTF_GetError());
    }
    SDL_Color rouge = {200, 0, 0};
    SDL_Color vert = {0, 200, 0};
    SDL_Color vert_clair = {140, 200, 140};
    fonts->rouge = rouge;
    fonts->vert = vert;
    fonts->vert_clair = vert_clair;
}

void destroyPlayer(Everything *all)
{
    if (all->player.texture != NULL)
    {
        SDL_DestroyTexture(all->player.texture);
        all->player.texture = NULL;
    }
    if (all->player.hitbox.points != NULL)
    {
        SDL_free(all->player.hitbox.points);
        all->player.hitbox.points = NULL;
    }
}

void destroyFirePlayer(FirePlayer *fire, Everything *all)
{
    FirePlayer *fire_liste = &all->liste_fireplayer, *tmp = NULL;
    if (fire == NULL)
    {
        fprintf(stderr, "Erreur dans destroyFirePlayer : le FirePlayer passé en paramètre est un pointeur NULL");
        return;
    }
    while (fire_liste->suivant != fire)
    {
        fire_liste = fire_liste->suivant;
    }
    if (fire_liste->suivant->hitbox.points != NULL)
    {
        SDL_free(fire_liste->suivant->hitbox.points);
    }
    if (fire_liste->suivant->texture != NULL)
    {
        SDL_DestroyTexture(fire_liste->suivant->texture);
    }
    tmp = fire_liste->suivant->suivant;
    SDL_free(fire_liste->suivant);
    fire_liste->suivant = tmp;
}

void destroyMob(Mob *mob, Everything *all)
{
    Mob *mob_liste = &all->liste_mob, *tmp = NULL;
    if (mob == NULL)
    {
        fprintf(stderr, "Erreur dans destroyMob : le Mob passé en paramètre est un pointeur NULL");
        return;
    }
    while (mob_liste->suivant != mob)
    {
        mob_liste = mob_liste->suivant;
    }
    if (mob_liste->suivant->hitbox.points != NULL)
    {
        SDL_free(mob_liste->suivant->hitbox.points);
    }
    if (mob_liste->suivant->texture != NULL)
    {
        SDL_DestroyTexture(mob_liste->suivant->texture);
    }
    tmp = mob_liste->suivant->suivant;
    SDL_free(mob_liste->suivant);
    mob_liste->suivant = tmp;
}

void destroyLevel(Everything *all)
{
    if (all->level.texture != NULL)
    {
        SDL_DestroyTexture(all->level.texture);
    }
    if (all->level.hitboxes != NULL)
    {
        for (int i = 0; i < all->level.nb_hitboxes; i++)
        {
            if (all->level.hitboxes[i].points != NULL)
            {
                SDL_free(all->level.hitboxes[i].points);
            }
        }
        SDL_free(all->level.hitboxes);
        all->level.nb_hitboxes = 0;
    }
}

void Quit(Everything *all, int statut)
{
    /* liberation de la RAM allouee */

    while (all->liste_text.suivant != NULL)
    {
        destroyText(all->liste_text.suivant, all);
    }
    destroyFonts(all);
    destroyLevel(all);
    destroyPlayer(all);
    while (all->liste_fireplayer.suivant != NULL)
    {
        destroyFirePlayer(all->liste_fireplayer.suivant, all);
    }
    while (all->liste_mob.suivant != NULL)
    {
        destroyMob(all->liste_mob.suivant, all);
    }

    /* destruction du renderer et de la fenetre, fermeture de la SDL puis sortie du programme */

    if (NULL != all->renderer)
        SDL_DestroyRenderer(all->renderer);
    if (NULL != all->window)
        SDL_DestroyWindow(all->window);
    TTF_Quit();
    SDL_Quit();
    printf("Libérations de toutes les ressources réussies\n");
    exit(statut);
}

void loadOptions(Everything *all)
{
    all->input.key_A = SDLK_SPACE;
    all->input.key_B = SDLK_d;
    all->input.key_down = SDLK_KP_8;
    all->input.key_L = SDLK_a;
    all->input.key_left = SDLK_KP_7;
    all->input.key_R = SDLK_z;
    all->input.key_right = SDLK_KP_9;
    all->input.key_select = SDLK_e;
    all->input.key_start = SDLK_RETURN;
    all->input.key_up = SDLK_KP_DIVIDE;
    all->input.select_on_cooldown = SDL_FALSE;
    all->input.start_on_cooldown = SDL_FALSE;
}

FirePlayer *loadFirePlayer(Everything *all)
{
    FirePlayer *last = &all->liste_fireplayer;
    while (last->suivant != NULL)
    {
        last = last->suivant;
    }
    last->suivant = SDL_malloc(sizeof(FirePlayer));
    last->suivant->texture = loadImage("data/fire_player.bmp", all->renderer);
    last->suivant->x = 0;
    last->suivant->y = 0;
    last->suivant->src_rect.h = 16;
    last->suivant->src_rect.w = 16;
    last->suivant->src_rect.x = 0;
    last->suivant->src_rect.y = 0;
    last->suivant->dst_rect.h = 16;
    last->suivant->dst_rect.w = 16;
    last->suivant->dst_rect.x = -8;
    last->suivant->dst_rect.y = -8;
    last->suivant->suivant = NULL;
    last->suivant->hitbox.cercle_x = 0;
    last->suivant->hitbox.cercle_y = 0;
    last->suivant->hitbox.cercle_rayon = 10;
    last->suivant->hitbox.nb_points = 4;
    SDL_Point points[4];
    last->suivant->hitbox.points = SDL_malloc(sizeof(points));
    last->suivant->hitbox.points[0].x = -2;
    last->suivant->hitbox.points[0].y = -3;
    last->suivant->hitbox.points[1].x = 2;
    last->suivant->hitbox.points[1].y = -3;
    last->suivant->hitbox.points[2].x = 2;
    last->suivant->hitbox.points[2].y = 3;
    last->suivant->hitbox.points[3].x = -2;
    last->suivant->hitbox.points[3].y = 3;
    return last->suivant;
}

void movePlayer(int x, int y, Everything *all)
{
    all->player.x += x;
    all->player.dst_rect.x += x;
    all->player.hitbox.cercle_x += x;
    for (int i = 0; i < all->player.hitbox.nb_points; i++)
    {
        all->player.hitbox.points[i].x += x;
    }
    for (int i = 0; i < all->level.nb_hitboxes; i++)
    {
        if (sat(&all->player.hitbox, &all->level.hitboxes[i]))
        {
            all->player.x -= x;
            all->player.dst_rect.x -= x;
            all->player.hitbox.cercle_x -= x;
            for (int j = 0; j < all->player.hitbox.nb_points; j++)
            {
                all->player.hitbox.points[j].x -= x;
            }
            break;
        }
    }

    all->player.y += y;
    all->player.dst_rect.y += y;
    all->player.hitbox.cercle_y += y;
    for (int i = 0; i < all->player.hitbox.nb_points; i++)
    {
        all->player.hitbox.points[i].y += y;
    }
    for (int i = 0; i < all->level.nb_hitboxes; i++)
    {
        if (sat(&all->player.hitbox, &all->level.hitboxes[i]))
        {
            all->player.y -= y;
            all->player.dst_rect.y -= y;
            all->player.hitbox.cercle_y -= y;
            for (int j = 0; j < all->player.hitbox.nb_points; j++)
            {
                all->player.hitbox.points[j].y -= y;
            }
            break;
        }
    }
}

void moveFirePlayer(int x, int y, FirePlayer *fire, Everything *all)
{
    fire->y += y;
    fire->x += x;
    fire->dst_rect.y += y;
    fire->dst_rect.x += x;
    fire->hitbox.cercle_y += y;
    fire->hitbox.cercle_x += x;
    for (int i = 0; i < fire->hitbox.nb_points; i++)
    {
        fire->hitbox.points[i].y += y;
        fire->hitbox.points[i].x += x;
    }
}

void moveMob(int x, int y, Mob *mob, Everything *all)
{
    mob->y += y;
    mob->x += x;
    mob->dst_rect.y += y;
    mob->dst_rect.x += x;
    mob->hitbox.cercle_y += y;
    mob->hitbox.cercle_x += x;
    for (int i = 0; i < mob->hitbox.nb_points; i++)
    {
        mob->hitbox.points[i].y += y;
        mob->hitbox.points[i].x += x;
    }
}

Mob *loadMob(Everything *all)
{
    Mob *last = &all->liste_mob;
    while (last->suivant != NULL)
    {
        last = last->suivant;
    }
    last->suivant = SDL_malloc(sizeof(Mob));
    last->suivant->texture = loadImage("data/ship_mob.bmp", all->renderer);
    last->suivant->src_rect.h = 16;
    last->suivant->src_rect.w = 16;
    last->suivant->src_rect.x = 0;
    last->suivant->src_rect.y = 0;
    last->suivant->dst_rect.h = 16;
    last->suivant->dst_rect.w = 16;
    last->suivant->dst_rect.x = -8;
    last->suivant->dst_rect.y = -8;
    last->suivant->x = 0;
    last->suivant->y = 0;
    last->suivant->suivant = NULL;
    last->suivant->PV = 1;
    last->suivant->hitbox.cercle_x = 0;
    last->suivant->hitbox.cercle_y = 0;
    last->suivant->hitbox.cercle_rayon = 10;
    last->suivant->hitbox.nb_points = 4;
    SDL_Point points[4];
    last->suivant->hitbox.points = SDL_malloc(sizeof(points));
    last->suivant->hitbox.points[0].x = -6;
    last->suivant->hitbox.points[0].y = -7;
    last->suivant->hitbox.points[1].x = 6;
    last->suivant->hitbox.points[1].y = -7;
    last->suivant->hitbox.points[2].x = 6;
    last->suivant->hitbox.points[2].y = 7;
    last->suivant->hitbox.points[3].x = -6;
    last->suivant->hitbox.points[3].y = 7;
    return last->suivant;
}

void spawnMobs(Everything *all)
{
    Mob *mob = loadMob(all);
    moveMob(160, 20, mob, all);
    mob = loadMob(all);
    moveMob(80, 20, mob, all);
    mob = loadMob(all);
    moveMob(240, 20, mob, all);
}

void firePlayer(Everything *all)
{
    FirePlayer *fire = loadFirePlayer(all);
    moveFirePlayer(all->player.x, all->player.y, fire, all);
    all->player.delay_fire = 10;
}

void loadPlayer(Everything *all)
{
    all->player.texture = loadImage("data/ship_player.bmp", all->renderer);
    all->player.src_rect.h = 16;
    all->player.src_rect.w = 16;
    all->player.src_rect.x = 0;
    all->player.src_rect.y = 0;
    all->player.dst_rect.h = 16;
    all->player.dst_rect.w = 16;
    all->player.dst_rect.x = 152;
    all->player.dst_rect.y = 172;
    all->player.x = 160;
    all->player.y = 180;
    all->player.hitbox.cercle_x = 160;
    all->player.hitbox.cercle_y = 180;
    all->player.hitbox.cercle_rayon = 10;
    all->player.hitbox.nb_points = 4;
    SDL_Point points[4];
    all->player.hitbox.points = SDL_malloc(sizeof(points));
    all->player.hitbox.points[0].x = 154;
    all->player.hitbox.points[0].y = 173;
    all->player.hitbox.points[1].x = 166;
    all->player.hitbox.points[1].y = 173;
    all->player.hitbox.points[2].x = 166;
    all->player.hitbox.points[2].y = 187;
    all->player.hitbox.points[3].x = 154;
    all->player.hitbox.points[3].y = 187;
    all->player.PV = 5;
    all->player.delay_fire = 0;
    all->player.invincibility_frames = 0;
    all->player.fire_on_cooldown = SDL_FALSE;
    all->player.invicible = SDL_FALSE;
    all->liste_fireplayer.suivant = NULL;
    all->liste_fireplayer.texture = NULL;
    all->liste_mob.suivant = NULL;
    all->liste_mob.texture = NULL;
}

void loadLevel(Everything *all)
{
    all->level.x = 0;
    all->level.y = 0;
    all->level.src_rect.h = 240;
    all->level.src_rect.w = 320;
    all->level.src_rect.x = 0;
    all->level.src_rect.y = 0;
    all->level.frame = 0;
    all->level.delay_button = 0;
    all->level.nb_hitboxes = 0;

    all->level.start_game.upward = &all->level.quit_game;
    all->level.start_game.to_the_left = &all->level.start_game;
    all->level.start_game.to_the_right = &all->level.start_game;
    all->level.start_game.downward = &all->level.settings;
    
    all->level.settings.upward = &all->level.start_game;
    all->level.settings.to_the_left = &all->level.settings;
    all->level.settings.to_the_right = &all->level.settings;
    all->level.settings.downward = &all->level.quit_game;
    
    all->level.quit_game.upward = &all->level.settings;
    all->level.quit_game.to_the_left = &all->level.quit_game;
    all->level.quit_game.to_the_right = &all->level.quit_game;
    all->level.quit_game.downward = &all->level.start_game;
    
    all->level.back_to_menu.upward = &all->level.control_settings;
    all->level.back_to_menu.to_the_left = &all->level.back_to_menu;
    all->level.back_to_menu.to_the_right = &all->level.back_to_menu;
    all->level.back_to_menu.downward = &all->level.control_settings;
    
    all->level.control_settings.upward = &all->level.back_to_menu;
    all->level.control_settings.to_the_left = &all->level.control_settings;
    all->level.control_settings.to_the_right = &all->level.control_settings;
    all->level.control_settings.downward = &all->level.back_to_menu;

    all->level.chg_up.upward = &all->level.chg_up;
    all->level.chg_up.to_the_left = &all->level.chg_up;
    all->level.chg_up.to_the_right = &all->level.chg_B;
    all->level.chg_up.downward = &all->level.chg_down;

    all->level.chg_down.upward = &all->level.chg_up;
    all->level.chg_down.to_the_left = &all->level.chg_down;
    all->level.chg_down.to_the_right = &all->level.chg_L;
    all->level.chg_down.downward = &all->level.chg_left;

    all->level.chg_left.upward = &all->level.chg_down;
    all->level.chg_left.to_the_left = &all->level.chg_left;
    all->level.chg_left.to_the_right = &all->level.chg_R;
    all->level.chg_left.downward = &all->level.chg_right;

    all->level.chg_right.upward = &all->level.chg_left;
    all->level.chg_right.to_the_left = &all->level.chg_right;
    all->level.chg_right.to_the_right = &all->level.chg_start;
    all->level.chg_right.downward = &all->level.chg_A;

    all->level.chg_A.upward = &all->level.chg_right;
    all->level.chg_A.to_the_left = &all->level.chg_A;
    all->level.chg_A.to_the_right = &all->level.chg_select;
    all->level.chg_A.downward = &all->level.control_back_to_settings;

    all->level.chg_B.upward = &all->level.chg_B;
    all->level.chg_B.to_the_left = &all->level.chg_up;
    all->level.chg_B.to_the_right = &all->level.chg_B;
    all->level.chg_B.downward = &all->level.chg_L;

    all->level.chg_L.upward = &all->level.chg_B;
    all->level.chg_L.to_the_left = &all->level.chg_down;
    all->level.chg_L.to_the_right = &all->level.chg_L;
    all->level.chg_L.downward = &all->level.chg_R;

    all->level.chg_R.upward = &all->level.chg_L;
    all->level.chg_R.to_the_left = &all->level.chg_left;
    all->level.chg_R.to_the_right = &all->level.chg_R;
    all->level.chg_R.downward = &all->level.chg_start;

    all->level.chg_start.upward = &all->level.chg_R;
    all->level.chg_start.to_the_left = &all->level.chg_right;
    all->level.chg_start.to_the_right = &all->level.chg_start;
    all->level.chg_start.downward = &all->level.chg_select;

    all->level.chg_select.upward = &all->level.chg_start;
    all->level.chg_select.to_the_left = &all->level.chg_A;
    all->level.chg_select.to_the_right = &all->level.chg_select;
    all->level.chg_select.downward = &all->level.control_back_to_settings;

    all->level.control_back_to_settings.upward = &all->level.chg_A;
    all->level.control_back_to_settings.to_the_left = &all->level.control_back_to_settings;
    all->level.control_back_to_settings.to_the_right = &all->level.control_back_to_settings;
    all->level.control_back_to_settings.downward = &all->level.control_back_to_settings;
}

void Init(Everything *all)
{
    /* initialisation de la SDL et de la TTF */

    if (0 != SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Erreur SDL_Init : %s\n", SDL_GetError());
        Quit(all, EXIT_FAILURE);
    }
    if (0 != TTF_Init())
    {
        fprintf(stderr, "Erreur TTF_Init : %s\n", TTF_GetError());
        Quit(all, EXIT_FAILURE);
    }

    /* creation de la fenetre et du renderer associé */

    all->window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              4*320, 4*240, SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/);
    if (NULL == all->window)
    {
        fprintf(stderr, "Erreur SDL_CreateWindow : %s\n", SDL_GetError());
        Quit(all, EXIT_FAILURE);
    }
    all->renderer = SDL_CreateRenderer(all->window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == all->renderer)
    {
        fprintf(stderr, "Erreur SDL_CreateRenderer : %s\n", SDL_GetError());
        Quit(all, EXIT_FAILURE);
    }
    SDL_RenderSetLogicalSize(all->renderer, 320, 240);

    /* chargement et affichage de l'icone de la fenetre */

    SDL_Surface *icone;
    icone = SDL_LoadBMP("icone.bmp");
    if (NULL == icone)
    {
        fprintf(stderr, "Erreur SDL_LoadBMP : %s\n", SDL_GetError());
        Quit(all, EXIT_FAILURE);
    }
    SDL_SetWindowIcon(all->window, icone);
    SDL_FreeSurface(icone);
}

void updateFirePlayers(Everything *all)
{
    FirePlayer *fire = &all->liste_fireplayer;
    int i;
    SDL_bool destroy = SDL_FALSE;
    while (fire->suivant != NULL)
    {
        moveFirePlayer(0, -3, fire->suivant, all);
        if (fire->suivant->texture != NULL)
        {
            SDL_RenderCopy(all->renderer, fire->suivant->texture, &fire->suivant->src_rect, &fire->suivant->dst_rect);
        }
        for (i = 0; i < all->level.nb_hitboxes-1; i++)
        {
            if (sat(&fire->suivant->hitbox, &all->level.hitboxes[i]))
            {
                destroy = SDL_TRUE;
            }
        }
        if (destroy)
        {
            destroyFirePlayer(fire->suivant, all);
            destroy = SDL_FALSE;
        }
        else
        {
            fire = fire->suivant;
        }
    }
}

void updatePlayer(Everything *all)
{
    /* update player position */

    if (all->input.down)
    {
        movePlayer(0, 1, all);
    }
    if (all->input.up)
    {
        movePlayer(0, -1, all);
    }
    if (all->input.right)
    {
        movePlayer(1, 0, all);
    }
    if (all->input.left)
    {
        movePlayer(-1, 0, all);
    }

    /* update player firing */

    if (all->player.delay_fire > 0)
    {
        all->player.delay_fire -= 1;
        all->player.fire_on_cooldown = SDL_TRUE;
    }
    else
    {
        all->player.fire_on_cooldown = SDL_FALSE;
    }

    if (all->input.B)
    {
        if (!all->player.fire_on_cooldown)
        {
            firePlayer(all);
        }
    }

    updateFirePlayers(all);

    /* update player PV */

    Mob *mob = &all->liste_mob;
    if (!all->player.invicible)
    {
        while (mob->suivant != NULL)
        {
            if (sat(&mob->suivant->hitbox, &all->player.hitbox))
            {
                if (!all->player.invicible)
                {
                    all->player.invicible = SDL_TRUE;
                    all->player.invincibility_frames = 40;
                    all->player.PV -= 1;
                }
            }
            mob = mob->suivant;
        }
    }
    else
    {
        all->player.invincibility_frames -= 1;
        if (all->player.invincibility_frames <= 0)
        {
            all->player.invincibility_frames = 0;
            all->player.invicible = SDL_FALSE;
        }
    }

    /* update player aff */

    if (!all->player.invicible)
    {
        if (all->player.texture != NULL)
        {
            SDL_RenderCopy(all->renderer, all->player.texture, &all->player.src_rect, &all->player.dst_rect);
        }
    }
    else
    {
        if (all->player.invincibility_frames % 2 == 0)
        {
            if (all->player.texture != NULL)
            {
                SDL_RenderCopy(all->renderer, all->player.texture, &all->player.src_rect, &all->player.dst_rect);
            }
        }
    }
}

void updateMobs(Everything *all)
{
    if (all->liste_mob.suivant == NULL)
    {
        spawnMobs(all);
    }

    Mob *mob = &all->liste_mob;
    FirePlayer *fire = &all->liste_fireplayer;
    int i;
    SDL_bool destroy = SDL_FALSE;
    while (mob->suivant != NULL)
    {
        moveMob(0, 1, mob->suivant, all);
        if (mob->suivant->texture != NULL)
        {
            SDL_RenderCopy(all->renderer, mob->suivant->texture, &mob->suivant->src_rect, &mob->suivant->dst_rect);
        }
        for (i = 0; i < all->level.nb_hitboxes-1; i++)
        {
            if (sat(&mob->suivant->hitbox, &all->level.hitboxes[i]))
            {
                destroy = SDL_TRUE;
            }
        }
        fire = &all->liste_fireplayer;
        while (fire->suivant != NULL)
        {
            if (sat(&mob->suivant->hitbox, &fire->suivant->hitbox))
            {
                mob->suivant->PV -= 1;
                destroyFirePlayer(fire->suivant, all);
            }
            else
            {
                fire = fire->suivant;
            }
        }
        if (mob->suivant->PV <= 0)
        {
            destroy = SDL_TRUE;
        }
        if (destroy)
        {
            destroyMob(mob->suivant, all);
            destroy = SDL_FALSE;
        }
        else
        {
            mob = mob->suivant;
        }
    }
}

void updateButton(Everything *all)
{
    Button *selected = all->level.selected_button;
    Input *input = &all->input;
    if (selected != NULL)
    {
        if (all->level.delay_button <= 0)
        {
            if (input->up)
            {
                selected = selected->upward;
                all->level.delay_button = 10;
            }
            if (input->down)
            {
                selected = selected->downward;
                all->level.delay_button = 10;
            }
            if (input->left)
            {
                selected = selected->to_the_left;
                all->level.delay_button = 10;
            }
            if (input->right)
            {
                selected = selected->to_the_right;
                all->level.delay_button = 10;
            }
        }
        else
        {
            all->level.delay_button -= 1;
        }
    }
    all->level.selected_button = selected;
}

void updateGame(Everything *all)
{
    /* On charge les ressources si elles ne sont pas déjà chargées */

    if (all->level.texture == NULL)
    {
        all->level.texture = loadImage("data/background.bmp", all->renderer);
    }
    if (all->level.hitboxes == NULL)
    {
        all->level.nb_hitboxes = 4;
        Hitbox hitboxes[4];
        all->level.hitboxes = SDL_malloc(sizeof(hitboxes));
        SDL_Point points[4];

        all->level.hitboxes[0].nb_points = 4;
        all->level.hitboxes[0].cercle_x = 160;
        all->level.hitboxes[0].cercle_y = -5;
        all->level.hitboxes[0].cercle_rayon = 170;
        all->level.hitboxes[0].points = SDL_malloc(sizeof(points));
        all->level.hitboxes[0].points[0].x = 0;
        all->level.hitboxes[0].points[0].y = -10;
        all->level.hitboxes[0].points[1].x = 320;
        all->level.hitboxes[0].points[1].y = -10;
        all->level.hitboxes[0].points[2].x = 320;
        all->level.hitboxes[0].points[2].y = 0;
        all->level.hitboxes[0].points[3].x = 0;
        all->level.hitboxes[0].points[3].y = 0;

        all->level.hitboxes[1].nb_points = 4;
        all->level.hitboxes[1].cercle_x = 325;
        all->level.hitboxes[1].cercle_y = 120;
        all->level.hitboxes[1].cercle_rayon = 130;
        all->level.hitboxes[1].points = SDL_malloc(sizeof(points));
        all->level.hitboxes[1].points[0].x = 320;
        all->level.hitboxes[1].points[0].y = 0;
        all->level.hitboxes[1].points[1].x = 330;
        all->level.hitboxes[1].points[1].y = 0;
        all->level.hitboxes[1].points[2].x = 330;
        all->level.hitboxes[1].points[2].y = 240;
        all->level.hitboxes[1].points[3].x = 320;
        all->level.hitboxes[1].points[3].y = 240;

        all->level.hitboxes[2].nb_points = 4;
        all->level.hitboxes[2].cercle_x = 160;
        all->level.hitboxes[2].cercle_y = 245;
        all->level.hitboxes[2].cercle_rayon = 170;
        all->level.hitboxes[2].points = SDL_malloc(sizeof(points));
        all->level.hitboxes[2].points[0].x = 0;
        all->level.hitboxes[2].points[0].y = 240;
        all->level.hitboxes[2].points[1].x = 320;
        all->level.hitboxes[2].points[1].y = 240;
        all->level.hitboxes[2].points[2].x = 320;
        all->level.hitboxes[2].points[2].y = 250;
        all->level.hitboxes[2].points[3].x = 0;
        all->level.hitboxes[2].points[3].y = 250;

        all->level.hitboxes[3].nb_points = 4;
        all->level.hitboxes[3].cercle_x = -5;
        all->level.hitboxes[3].cercle_y = 120;
        all->level.hitboxes[3].cercle_rayon = 130;
        all->level.hitboxes[3].points = SDL_malloc(sizeof(points));
        all->level.hitboxes[3].points[0].x = -10;
        all->level.hitboxes[3].points[0].y = 0;
        all->level.hitboxes[3].points[1].x = 0;
        all->level.hitboxes[3].points[1].y = 0;
        all->level.hitboxes[3].points[2].x = 0;
        all->level.hitboxes[3].points[2].y = 240;
        all->level.hitboxes[3].points[3].x = -10;
        all->level.hitboxes[3].points[3].y = 240;
    }

    /* Affichage du Level */

    if (all->level.frame >= 3)
    {
        all->level.src_rect.y -= 1;
        if (all->level.src_rect.y <= 0)
        {
            all->level.src_rect.y = 480;
        }
        all->level.frame = 0;
    }
    all->level.frame += 1;
    if (all->level.texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.texture, &all->level.src_rect, NULL);
    }
}

void updateMainmenu(Everything *all)
{
    /* On charge les ressources si elles ne sont pas déjà chargées */
    
    if (all->level.texture == NULL)
    {
        all->level.texture = loadImage("data/background.bmp", all->renderer);
    }
    if (all->level.title == NULL)
    {
        all->level.title = loadText(all->fonts.titles, "Space Shooter", all->fonts.vert, all);
        all->level.title->dst_rect.x = 160 - (all->level.title->dst_rect.w / 2);
        all->level.title->dst_rect.y += 53;
    }
    if (all->level.start_game.text == NULL)
    {
        all->level.start_game.text = loadText(all->fonts.menu_button, "Start Game", all->fonts.vert_clair, all);
        all->level.start_game.text->dst_rect.x = 160 - (all->level.start_game.text->dst_rect.w / 2);
        all->level.start_game.text->dst_rect.y += 130;
    }
    if (all->level.settings.text == NULL)
    {
        all->level.settings.text = loadText(all->fonts.menu_button, "Options", all->fonts.vert_clair, all);
        all->level.settings.text->dst_rect.x = 160 - (all->level.settings.text->dst_rect.w / 2);
        all->level.settings.text->dst_rect.y += 160;
    }
    if (all->level.quit_game.text == NULL)
    {
        all->level.quit_game.text = loadText(all->fonts.menu_button, "Quit Game", all->fonts.vert_clair, all);
        all->level.quit_game.text->dst_rect.x = 160 - (all->level.quit_game.text->dst_rect.w / 2);
        all->level.quit_game.text->dst_rect.y += 190;
    }
    if (all->level.selected_button == NULL)
    {
        all->level.selected_button = &all->level.start_game;
    }
    updateButton(all);

    /* Affichage du Menu Principal */

    if (all->level.texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.texture, &all->level.src_rect, NULL);
    }
    if (all->level.title->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.title->texture, NULL, &all->level.title->dst_rect);
    }
    if (all->level.start_game.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.start_game.text->texture, NULL, &all->level.start_game.text->dst_rect);
    }
    if (all->level.settings.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.settings.text->texture, NULL, &all->level.settings.text->dst_rect);
    }
    if (all->level.quit_game.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.quit_game.text->texture, NULL, &all->level.quit_game.text->dst_rect);
    }
}

void updateGameover(Everything *all)
{
    /* On charge les ressources si elles ne sont pas déjà chargées */

    if (all->level.texture == NULL)
    {
        all->level.texture = loadImage("data/background.bmp", all->renderer);
    }
    if (all->level.game_over == NULL)
    {
        all->level.game_over = loadText(all->fonts.titles, "Game Over", all->fonts.rouge, all);
        all->level.game_over->dst_rect.x = 160 - (all->level.game_over->dst_rect.w / 2);
        all->level.game_over->dst_rect.y += 95;
    }

    /* Affichage de l'écran de Game Over */

    if (all->level.texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.texture, &all->level.src_rect, NULL);
    }
    if (all->level.game_over->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.game_over->texture, NULL, &all->level.game_over->dst_rect);
    }
}

void updateSettings(Everything *all)
{
    /* On charge les ressources si elles ne sont pas déjà chargées */
    
    if (all->level.texture == NULL)
    {
        all->level.texture = loadImage("data/background.bmp", all->renderer);
    }
    if (all->level.settings_title == NULL)
    {
        all->level.settings_title = loadText(all->fonts.secondary_titles, "Options", all->fonts.vert_clair, all);
        all->level.settings_title->dst_rect.x = 160 - (all->level.settings_title->dst_rect.w / 2);
        all->level.settings_title->dst_rect.y = 50;
    }
    if (all->level.control_settings.text == NULL)
    {
        all->level.control_settings.text = loadText(all->fonts.menu_button, "Keyboard Settings", all->fonts.vert_clair, all);
        all->level.control_settings.text->dst_rect.x = 160 - (all->level.control_settings.text->dst_rect.w / 2);
        all->level.control_settings.text->dst_rect.y = 130;
    }
    if (all->level.back_to_menu.text == NULL)
    {
        all->level.back_to_menu.text = loadText(all->fonts.menu_button, "Back To Menu", all->fonts.vert_clair, all);
        all->level.back_to_menu.text->dst_rect.x = 160 - (all->level.back_to_menu.text->dst_rect.w / 2);
        all->level.back_to_menu.text->dst_rect.y = 160;
    }
    if (all->level.selected_button == NULL)
    {
        all->level.selected_button = &all->level.control_settings;
    }
    updateButton(all);

    /* Affichage du Menu Principal */

    if (all->level.texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.texture, &all->level.src_rect, NULL);
    }
    if (all->level.settings_title->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.settings_title->texture, NULL, &all->level.settings_title->dst_rect);
    }
    if (all->level.control_settings.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.control_settings.text->texture, NULL, &all->level.control_settings.text->dst_rect);
    }
    if (all->level.back_to_menu.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.back_to_menu.text->texture, NULL, &all->level.back_to_menu.text->dst_rect);
    }
}

void updateControlsettings(Everything *all)
{
    /* On charge les ressources si elles ne sont pas déjà chargées */
    
    if (all->level.texture == NULL)
    {
        all->level.texture = loadImage("data/background.bmp", all->renderer);
    }
    if (all->level.control_settings_title == NULL)
    {
        all->level.control_settings_title = loadText(all->fonts.secondary_titles, "Keyboard Settings", all->fonts.vert_clair, all);
        all->level.control_settings_title->dst_rect.x = 160 - (all->level.control_settings_title->dst_rect.w / 2);
        all->level.control_settings_title->dst_rect.y = 20;
    }
    if (all->level.chg_up.text == NULL)
    {
        all->level.chg_up.text = loadText(all->fonts.menu_button, "Up", all->fonts.vert_clair, all);
        all->level.chg_up.text->dst_rect.x = 40;
        all->level.chg_up.text->dst_rect.y = 60;
    }
    if (all->level.chg_down.text == NULL)
    {
        all->level.chg_down.text = loadText(all->fonts.menu_button, "Down", all->fonts.vert_clair, all);
        all->level.chg_down.text->dst_rect.x = 40;
        all->level.chg_down.text->dst_rect.y = 80;
    }
    if (all->level.chg_left.text == NULL)
    {
        all->level.chg_left.text = loadText(all->fonts.menu_button, "Left", all->fonts.vert_clair, all);
        all->level.chg_left.text->dst_rect.x = 40;
        all->level.chg_left.text->dst_rect.y = 100;
    }
    if (all->level.chg_right.text == NULL)
    {
        all->level.chg_right.text = loadText(all->fonts.menu_button, "Right", all->fonts.vert_clair, all);
        all->level.chg_right.text->dst_rect.x = 40;
        all->level.chg_right.text->dst_rect.y = 120;
    }
    if (all->level.chg_A.text == NULL)
    {
        all->level.chg_A.text = loadText(all->fonts.menu_button, "A", all->fonts.vert_clair, all);
        all->level.chg_A.text->dst_rect.x = 40;
        all->level.chg_A.text->dst_rect.y = 140;
    }
    if (all->level.chg_B.text == NULL)
    {
        all->level.chg_B.text = loadText(all->fonts.menu_button, "B", all->fonts.vert_clair, all);
        all->level.chg_B.text->dst_rect.x = 200;
        all->level.chg_B.text->dst_rect.y = 60;
    }
    if (all->level.chg_L.text == NULL)
    {
        all->level.chg_L.text = loadText(all->fonts.menu_button, "L", all->fonts.vert_clair, all);
        all->level.chg_L.text->dst_rect.x = 200;
        all->level.chg_L.text->dst_rect.y = 80;
    }
    if (all->level.chg_R.text == NULL)
    {
        all->level.chg_R.text = loadText(all->fonts.menu_button, "R", all->fonts.vert_clair, all);
        all->level.chg_R.text->dst_rect.x = 200;
        all->level.chg_R.text->dst_rect.y = 100;
    }
    if (all->level.chg_start.text == NULL)
    {
        all->level.chg_start.text = loadText(all->fonts.menu_button, "Start", all->fonts.vert_clair, all);
        all->level.chg_start.text->dst_rect.x = 200;
        all->level.chg_start.text->dst_rect.y = 120;
    }
    if (all->level.chg_select.text == NULL)
    {
        all->level.chg_select.text = loadText(all->fonts.menu_button, "Select", all->fonts.vert_clair, all);
        all->level.chg_select.text->dst_rect.x = 200;
        all->level.chg_select.text->dst_rect.y = 140;
    }
    if (all->level.control_back_to_settings.text == NULL)
    {
        all->level.control_back_to_settings.text = loadText(all->fonts.menu_button, "Save and Exit", all->fonts.vert_clair, all);
        all->level.control_back_to_settings.text->dst_rect.x = 160 - (all->level.control_back_to_settings.text->dst_rect.w / 2);
        all->level.control_back_to_settings.text->dst_rect.y = 200;
    }
    if (all->level.selected_button == NULL)
    {
        all->level.selected_button = &all->level.chg_up;
    }

    /* gestion des changements de touches de controles */

    if (!all->input.start && !all->input.waiting_for_input)
    {
        updateButton(all);
    }
    else
    {
        if (all->input.waiting_for_input && all->input.wanted_input != -1)
        {
            if (all->level.selected_button == &all->level.chg_up)
            {
                all->input.key_up = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_down)
            {
                all->input.key_down = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_left)
            {
                all->input.key_left = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_right)
            {
                all->input.key_right = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_A)
            {
                all->input.key_A = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_B)
            {
                all->input.key_B = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_L)
            {
                all->input.key_L = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_R)
            {
                all->input.key_R = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_select)
            {
                all->input.key_select = all->input.wanted_input;
            }
            else if (all->level.selected_button == &all->level.chg_start)
            {
                all->input.key_start = all->input.wanted_input;
            }
            all->input.waiting_for_input = SDL_FALSE;
        }
        else if (all->level.selected_button != &all->level.control_back_to_settings)
        {
            all->input.waiting_for_input = SDL_TRUE;
        }
    }

    /* Affichage du Menu Principal */

    if (all->level.texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.texture, &all->level.src_rect, NULL);
    }
    if (all->level.control_settings_title->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.control_settings_title->texture, NULL, &all->level.control_settings_title->dst_rect);
    }
    if (all->level.chg_up.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_up.text->texture, NULL, &all->level.chg_up.text->dst_rect);
    }
    if (all->level.chg_down.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_down.text->texture, NULL, &all->level.chg_down.text->dst_rect);
    }
    if (all->level.chg_left.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_left.text->texture, NULL, &all->level.chg_left.text->dst_rect);
    }
    if (all->level.chg_right.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_right.text->texture, NULL, &all->level.chg_right.text->dst_rect);
    }
    if (all->level.chg_A.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_A.text->texture, NULL, &all->level.chg_A.text->dst_rect);
    }
    if (all->level.chg_B.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_B.text->texture, NULL, &all->level.chg_B.text->dst_rect);
    }
    if (all->level.chg_L.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_L.text->texture, NULL, &all->level.chg_L.text->dst_rect);
    }
    if (all->level.chg_R.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_R.text->texture, NULL, &all->level.chg_R.text->dst_rect);
    }
    if (all->level.chg_start.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_start.text->texture, NULL, &all->level.chg_start.text->dst_rect);
    }
    if (all->level.chg_select.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.chg_select.text->texture, NULL, &all->level.chg_select.text->dst_rect);
    }
    if (all->level.control_back_to_settings.text->texture != NULL)
    {
        SDL_RenderCopy(all->renderer, all->level.control_back_to_settings.text->texture, NULL, &all->level.control_back_to_settings.text->dst_rect);
    }
}

void updateGameState(Everything *all)
{
    switch (all->game_state)
    {
        case 0 :        /* Title Screen */
        {
            if (all->input.start)
            {
                if (all->level.selected_button == &all->level.start_game)
                {
                    /* Start Game */
                    all->game_state = 1;
                    loadPlayer(all);
                    if (all->level.title != NULL)
                    {
                        destroyText(all->level.title, all);
                        all->level.title = NULL;
                    }
                    if (all->level.start_game.text != NULL)
                    {
                        destroyText(all->level.start_game.text, all);
                        all->level.start_game.text = NULL;
                    }
                    if (all->level.settings.text != NULL)
                    {
                        destroyText(all->level.settings.text, all);
                        all->level.settings.text = NULL;
                    }
                    if (all->level.quit_game.text != NULL)
                    {
                        destroyText(all->level.quit_game.text, all);
                        all->level.quit_game.text = NULL;
                    }
                    all->level.selected_button = NULL;
                }
                else if (all->level.selected_button == &all->level.settings)
                {
                    /* Go to the Settings Menu */
                    all->game_state = 3;
                    if (all->level.title != NULL)
                    {
                        destroyText(all->level.title, all);
                        all->level.title = NULL;
                    }
                    if (all->level.start_game.text != NULL)
                    {
                        destroyText(all->level.start_game.text, all);
                        all->level.start_game.text = NULL;
                    }
                    if (all->level.settings.text != NULL)
                    {
                        destroyText(all->level.settings.text, all);
                        all->level.settings.text = NULL;
                    }
                    if (all->level.quit_game.text != NULL)
                    {
                        destroyText(all->level.quit_game.text, all);
                        all->level.quit_game.text = NULL;
                    }
                    all->level.selected_button = NULL;
                }
                else if (all->level.selected_button == &all->level.quit_game)
                {
                    /* Quit Game */
                    all->input.quit = SDL_TRUE;
                    all->level.selected_button = NULL;
                }
            }
            break;
        }
        case 1 :        /* Game */
        {
            if (all->player.PV <= 0)
            {
                /* Game Over */
                all->game_state = 2;
                destroyPlayer(all);
                if (all->level.hitboxes != NULL)
                {
                    for (int i = 0; i < all->level.nb_hitboxes; i++)
                    {
                        if (all->level.hitboxes[i].points != NULL)
                        {
                            SDL_free(all->level.hitboxes[i].points);
                        }
                    }
                    SDL_free(all->level.hitboxes);
                    all->level.hitboxes = NULL;
                    all->level.nb_hitboxes = 0;
                }
                while (all->liste_fireplayer.suivant != NULL)
                {
                    destroyFirePlayer(all->liste_fireplayer.suivant, all);
                }
                while (all->liste_mob.suivant != NULL)
                {
                    destroyMob(all->liste_mob.suivant, all);
                }
            }
            break;
        }
        case 2 :        /* Game Over Screen */
        {
            if (all->input.start)
            {
                /* Return to Menu */
                all->game_state = 0;
                if (all->level.game_over != NULL)
                {
                    destroyText(all->level.game_over, all);
                    all->level.game_over = NULL;
                }
            }
            break;
        }
        case 3 :        /* Settings Menu */
        {
            if (all->input.start)
            {
                if (all->level.selected_button == &all->level.control_settings)
                {
                    /* Go to the Control Settings */
                    all->game_state = 4;
                    if (all->level.settings_title != NULL)
                    {
                        destroyText(all->level.settings_title, all);
                        all->level.settings_title = NULL;
                    }
                    if (all->level.back_to_menu.text != NULL)
                    {
                        destroyText(all->level.back_to_menu.text, all);
                        all->level.back_to_menu.text = NULL;
                    }
                    if (all->level.control_settings.text != NULL)
                    {
                        destroyText(all->level.control_settings.text, all);
                        all->level.control_settings.text = NULL;
                    }
                    all->level.selected_button = NULL;
                }
                else if (all->level.selected_button == &all->level.back_to_menu)
                {
                    /* Go back to the Main Menu */
                    all->game_state = 0;
                    if (all->level.settings_title != NULL)
                    {
                        destroyText(all->level.settings_title, all);
                        all->level.settings_title = NULL;
                    }
                    if (all->level.back_to_menu.text != NULL)
                    {
                        destroyText(all->level.back_to_menu.text, all);
                        all->level.back_to_menu.text = NULL;
                    }
                    if (all->level.control_settings.text != NULL)
                    {
                        destroyText(all->level.control_settings.text, all);
                        all->level.control_settings.text = NULL;
                    }
                    all->level.selected_button = NULL;
                }
            }
        }
        case 4 :        /* Control Settings */
        {
            if (all->input.start)
            {
                if (all->level.selected_button == &all->level.control_back_to_settings)
                {
                    /* Go to Settings */
                    all->game_state = 3;
                    if (all->level.control_settings_title != NULL)
                    {
                        destroyText(all->level.control_settings_title, all);
                        all->level.control_settings_title = NULL;
                    }
                    if (all->level.chg_up.text != NULL)
                    {
                        destroyText(all->level.chg_up.text, all);
                        all->level.chg_up.text = NULL;
                    }
                    if (all->level.chg_down.text != NULL)
                    {
                        destroyText(all->level.chg_down.text, all);
                        all->level.chg_down.text = NULL;
                    }
                    if (all->level.chg_left.text != NULL)
                    {
                        destroyText(all->level.chg_left.text, all);
                        all->level.chg_left.text = NULL;
                    }
                    if (all->level.chg_right.text != NULL)
                    {
                        destroyText(all->level.chg_right.text, all);
                        all->level.chg_right.text = NULL;
                    }
                    if (all->level.chg_A.text != NULL)
                    {
                        destroyText(all->level.chg_A.text, all);
                        all->level.chg_A.text = NULL;
                    }
                    if (all->level.chg_B.text != NULL)
                    {
                        destroyText(all->level.chg_B.text, all);
                        all->level.chg_B.text = NULL;
                    }
                    if (all->level.chg_L.text != NULL)
                    {
                        destroyText(all->level.chg_L.text, all);
                        all->level.chg_L.text = NULL;
                    }
                    if (all->level.chg_R.text != NULL)
                    {
                        destroyText(all->level.chg_R.text, all);
                        all->level.chg_R.text = NULL;
                    }
                    if (all->level.chg_start.text != NULL)
                    {
                        destroyText(all->level.chg_start.text, all);
                        all->level.chg_start.text = NULL;
                    }
                    if (all->level.chg_select.text != NULL)
                    {
                        destroyText(all->level.chg_select.text, all);
                        all->level.chg_select.text = NULL;
                    }
                    if (all->level.control_back_to_settings.text != NULL)
                    {
                        destroyText(all->level.control_back_to_settings.text, all);
                        all->level.control_back_to_settings.text = NULL;
                    }
                    all->level.selected_button = NULL;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    /* Création des variables */

    Everything all = {.renderer = NULL, .window = NULL};
    all.input.quit = SDL_FALSE;
    all.game_state = 0;
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        all.input.key[i] = SDL_FALSE;

    /* Initialisation, création de la fenêtre et du renderer. */

    Init(&all);

    /* Chargement des options et du level */

    loadFonts(&all);
    loadOptions(&all);
    loadLevel(&all);

    /* Boucle principale du jeu */

    while (!all.input.quit)
    {
        updateEvent(&all.input);
        SDL_RenderClear(all.renderer);

        switch (all.game_state)
        {   
            case 0 :        /* Title Screen */
            {
                updateMainmenu(&all);
                break;
            };
            case 1 :       /* Game */
            {
                updateGame(&all);
                updatePlayer(&all);
                updateMobs(&all);
                break;
            }
            case 2 :       /* Game Over Screen */
            {
                updateGameover(&all);
                break;
            }
            case 3 :        /* Settings Menu */
            {
                updateSettings(&all);
                break;
            }
            case 4 :        /* Control Settings */
            {
                updateControlsettings(&all);
                break;
            }
        }
        updateGameState(&all);
        
        SDL_RenderPresent(all.renderer);
        SDL_Delay( (int)(1000 / 60) );
    }

    /* Fermeture du logiciel et libération de la mémoire */

    Quit(&all, EXIT_SUCCESS);
}