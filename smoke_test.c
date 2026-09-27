/* =========================================================================
 * libGT — SMOKE TEST GLOBAL (fenetre + dessin + vec2 + couleurs + input
 *          + QPC + renderer + ECS complet + timers + Input Mapping)
 * =========================================================================
 * Couverture : TOUTE l'API publique, chaque fonction au moins une fois.
 * Une fenetre s'affiche ~1 s (section 8) : c'est normal.
 *
 * gcc -std=c11 -Wall -Wextra -O2 smoke_test.c -o smoke_test.exe -lgdi32 -luser32
 * ./smoke_test.exe
 * ========================================================================= */
#define LIBGT_IMPLEMENTATION
#include "libGT.h"
#include <windows.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>

static int failures = 0, checks = 0, s_start = 0;
#define CHECK(cond, msg) do { checks++; \
    if (!(cond)) { failures++; printf("      FAIL: %s\n", msg); } } while (0)
static void begin(const char* n) { s_start = checks; printf("  [%-20s] ", n); }
static void end(void)   { printf("OK (%d checks)\n", checks - s_start); }

static void cbTick(void* ud, uint32_t id) { (void)id; (*(int*)ud)++; }
static void sysCount(GtECS* ecs, GtEntity e, void* ud) { (void)ecs; (void)e; (*(int*)ud)++; }
static void onHit(GtECS* ecs, GtEntity a, GtEntity b, void* ud) { (void)ecs; (void)a; (void)b; (*(int*)ud)++; }

int main(void) {
    printf("=== libGT SMOKE TEST ===\n");

    /* ---------- 1. GtVec2 ---------- */
    begin("1. GtVec2");
    {
        GtVec2 a = gtVec2(3, 4);
        CHECK(gtVec2Len2(a) == 25.0f, "Len2(3,4)=25");
        CHECK(gtVec2Len(a) == 5.0f, "Len(3,4)=5");
        GtVec2 n = gtVec2Norm(a);
        CHECK(fabsf(gtVec2Len(n) - 1.0f) < 1e-6f, "Norm -> unitaire");
        CHECK(gtVec2Len(gtVec2Norm(gtVec2(0, 0))) == 0.0f, "Norm(0,0)=(0,0)");
        GtVec2 l = gtVec2Lerp(gtVec2(0, 0), gtVec2(10, 10), 0.5f);
        CHECK(l.x == 5.0f && l.y == 5.0f, "Lerp t=0.5");
        CHECK(gtVec2Dot(gtVec2(1, 0), gtVec2(0, 1)) == 0.0f, "Dot orthogonal");
        CHECK(gtVec2Sub(gtVec2(1, 2), gtVec2(1, 2)).x == 0.0f, "Sub");
        CHECK(gtVec2Div(gtVec2(4, 6), 2.0f).y == 3.0f, "Div");
        CHECK(gtVec2Mul(gtVec2(2, 3), 2.0f).x == 4.0f, "Mul");
        CHECK(gtVec2Add(gtVec2(1, 1), gtVec2(2, 2)).y == 3.0f, "Add");
    }
    end();

    /* ---------- 2. Couleurs ---------- */
    begin("2. Couleurs");
    {
        CHECK(gtColorRGBA(0x12, 0x34, 0x56, 0xAB) == 0xAB123456u, "RGBA packing");
        CHECK(gtColorRGB(0, 0, 0) == 0xFF000000u, "RGB alpha force 255");
        CHECK(GT_RED != GT_GREEN && GT_GREEN != GT_BLUE, "constantes distinctes");
        CHECK(GT_BLACK == gtColorRGB(0, 0, 0), "GT_BLACK");
        CHECK(GT_WHITE == gtColorRGB(255, 255, 255), "GT_WHITE");
        CHECK(GT_DARKGRAY != 0, "GT_DARKGRAY non nul");
    }
    end();

    /* ---------- 3. Temps (QPC) ---------- */
    begin("3. Temps (QPC)");
    {
        gtBeginFrame();
        CHECK(gtGetDeltaTime() == 0.0f, "dt=0 au premier begin");
        double t0 = gtGetTime();
        CHECK(t0 >= 0.0, "GetTime >= 0");
        Sleep(20);
        gtBeginFrame();
        float dt = gtGetDeltaTime();
        CHECK(dt > 0.0f && dt < 1.0f, "dt ~20ms");
        CHECK(gtGetTime() > t0, "temps monotone");
        CHECK(gtGetDeltaTime() == dt, "dt stable dans la frame");
    }
    end();

    /* ---------- 4. Timers ---------- */
    begin("4. Timers");
    {
        gtClearAllTimers();
        int fired = 0;
        uint32_t id = gtSetTimeout(cbTick, 0.1f, &fired);
        CHECK(id != 0, "setTimeout -> id");
        CHECK(gtTimerActive(id), "actif a la creation");
        gtTimersUpdate(0.05f); CHECK(fired == 0, "pas avant echeance");
        gtTimersUpdate(0.06f); CHECK(fired == 1, "tire a l'echeance");
        CHECK(!gtTimerActive(id), "one-shot mort apres tir");
        uint32_t iv = gtSetInterval(cbTick, 0.1f, &fired);
        gtTimersUpdate(0.25f);
        CHECK(fired == 2, "interval: +1 tir max/update (garantie no-burst)");
        CHECK(gtClearTimer(iv), "clear interval");
        CHECK(!gtClearTimer(iv), "double clear = no-op");
        CHECK(gtSetTimeout(NULL, 1, NULL) == 0, "func NULL refuse");
        CHECK(gtSetTimeout(cbTick, NAN, NULL) == 0, "NaN refuse");
        CHECK(gtSetTimeout(cbTick, -1, NULL) == 0, "delai negatif refuse");
        CHECK(!gtTimerActive(0) && !gtTimerActive(999999), "ids invalides inactifs");
        gtClearAllTimers();
    }
    end();

    /* ---------- 5. ECS ---------- */
    begin("5. ECS");
    {
        GtECS ecs;
        gtECSInit(&ecs, 32);
        CHECK(ecs.capacity == 32, "init capacite 32");
        GtEntity a = gtECSCreateEntity(&ecs);
        GtEntity b = gtECSCreateEntity(&ecs);
        CHECK(a != GT_ENTITY_NULL && b != GT_ENTITY_NULL && a != b, "2 entites distinctes");
        gtECSAddTransform(&ecs, a, gtVec2(100, 100), 0, 1);
        gtECSAddVelocity(&ecs, a, gtVec2(10, 0), 0);
        gtECSAddCircleCollider(&ecs, a, 5, gtVec2(0, 0), 1u, 0xFFFFFFFFu);
        gtECSAddHealth(&ecs, a, 2);
        gtECSAddTag(&ecs, a, GT_COMP_PLAYER);
        gtECSAddTransform(&ecs, b, gtVec2(105, 100), 0, 1);
        gtECSAddCircleCollider(&ecs, b, 5, gtVec2(0, 0), 1u, 0xFFFFFFFFu);
        CHECK(gtECSIsAlive(&ecs, a), "alive");
        CHECK(!gtECSIsAlive(&ecs, GT_ENTITY_MAKE(9999, 1)), "index hors bornes");
        CHECK(gtECSFindFirstByMask(&ecs, GT_COMP_PLAYER) == a, "find player");
        CHECK(gtECSGetTransform(&ecs, a) != NULL, "get transform");
        CHECK(gtECSGetVelocity(&ecs, b) == NULL, "b n'a pas de velocity");
        gtECSSystemVelocity(&ecs, 1.0f);
        CHECK(gtECSGetTransform(&ecs, a)->pos.x == 110.0f, "velocity: x 100->110");
        CHECK(gtECSAddVelocity(&ecs, b, gtVec2(0, NAN), 0) == NULL, "NaN refuse");
        int hits = 0;
        gtECSSystemCollisions(&ecs, onHit, &hits);
        CHECK(hits == 1, "1 paire en collision");
        CHECK(!gtECSDamageEntity(&ecs, a, 1), "1er degat non fatal");
        gtECSSystemHealthInvincibility(&ecs, 0.6f);   /* purge l'invincibilite 0.5s */
        CHECK(gtECSDamageEntity(&ecs, a, 1), "2e degat fatal (apres invincibilite)");
        CHECK(gtECSIsAlive(&ecs, a), "damage ne detruit pas (purge differree)");
        CHECK(gtECSSystemHealthDeath(&ecs) == 1, "HealthDeath purge le mort");
        CHECK(!gtECSIsAlive(&ecs, a), "purge effective");
        CHECK(!gtECSAddTag(&ecs, a, GT_COMP_ENEMY), "tag sur morte refuse");
        CHECK(gtECSFindFirstByMask(&ecs, GT_COMP_PLAYER) == GT_ENTITY_NULL, "player introuvable apres mort");
        int count = 0;
        gtECSRunSystem(&ecs, GT_COMP_TRANSFORM, sysCount, &count);
        CHECK(count == 1, "RunSystem: 1 transform restant (b)");
        gtECSAddLifetime(&ecs, b, 0.1f);
        gtECSSystemLifetime(&ecs, 0.2f);
        CHECK(!gtECSIsAlive(&ecs, b), "lifetime ecoule -> b detruit");
        gtECSDestroyEntity(&ecs, b);   // double destroy: doit etre no-op
        CHECK(true, "double destroy sans crash");
        gtECSDestroy(&ecs);
        CHECK(true, "destroy");
    }
    end();

    /* ---------- 6. Fenetre + dessin + clipping ---------- */
    begin("6. Fenetre + dessin");
    {
        GtWindow* w = gtCreateWindow("smoke", 320, 200);
        CHECK(w != NULL, "creation 320x200");
        CHECK(gtGetWidth(w) == 320 && gtGetHeight(w) == 200, "dimensions");
        CHECK(gtCreateWindow(NULL, 100, 100) == NULL, "titre NULL refuse");
        CHECK(gtCreateWindow("x", 0, 100) == NULL, "largeur 0 refusee");
        CHECK(gtCreateWindow("x", 100, -5) == NULL, "hauteur negative refusee");
        /* Overflow ARITHMETIQUE rejeté : INT_MAX*INT_MAX*4 > SIZE_MAX.
         * (Une allocation 40 Go peut légitimement réussir sur Windows :
         *  pages zeroed lazy — ce n'est PAS un overflow, pas un bug.) */
        CHECK(gtCreateWindow("ovf", INT_MAX, INT_MAX) == NULL, "overflow w*h*4 > SIZE_MAX refuse");

        CHECK(gtEventsWindow(w), "events: pas de quit");
        CHECK(!gtShouldClose(w), "pas de fermeture spontanee");

        gtClearWindow(w, GT_DARKGRAY);
        gtDrawPixel(w, 10, 10, GT_RED);
        gtDrawRect(w, 20, 20, 50, 30, GT_GREEN);
        gtDrawRectLines(w, 100, 20, 40, 40, GT_BLUE);
        gtDrawLine(w, 0, 0, 319, 199, GT_YELLOW);
        gtDrawCircle(w, 160, 100, 30, GT_RED);
        gtDrawCircleLines(w, 160, 100, 50, GT_WHITE);
        CHECK(true, "primitives normales sans crash");

        /* Clipping extremes : ne doit ni crasher ni corrompre */
        gtDrawRect(w, -1000, -1000, 5000, 5000, gtColorRGBA(255, 0, 0, 20));
        gtDrawRect(w, INT_MIN / 2, 10, INT_MAX / 2, 20, GT_RED);
        gtDrawRect(w, 50, 50, -100, 100, GT_RED);        /* w negatif ignore */
        gtDrawRect(w, 50, 50, 100, 0, GT_RED);           /* h=0 ignore */
        gtDrawLine(w, INT_MIN, INT_MIN, INT_MAX, INT_MAX, GT_WHITE);
        gtDrawLine(w, -5000, -5000, 5000, 5000, GT_GREEN);
        gtDrawCircle(w, -400, -400, 300, GT_BLUE);       /* centre hors ecran */
        gtDrawCircleLines(w, 160, 100, 100000, GT_YELLOW);/* rayon>>ecran, clipped */
        gtDrawPixel(w, -1, -1, GT_RED);
        gtDrawPixel(w, 10000, 10000, GT_RED);
        CHECK(true, "coords extremes sans crash");

        /* Inputs : etats invalides */
        CHECK(!gtIsKeyDown(w, 0) && !gtIsKeyDown(w, 255), "keys bornes");
        CHECK(!gtIsKeyDown(w, 256) && !gtIsKeyDown(w, -1), "keys hors bornes");
        CHECK(!gtIsMouseButtonDown(w, -1) && !gtIsMouseButtonDown(w, 3), "mouse hors bornes");
        gtGetMousePos(w, NULL, NULL); /* outs NULL : no crash */
        int mx = -999, my = -999;
        gtGetMousePos(w, &mx, &my);
        CHECK(true, "getMousePos sans crash");

        gtUpdateWindow(w);  /* vrai blit */
        CHECK(true, "blit sans crash");

        /* Renderer */
        GtRenderer* r = gtCreateRenderer(w, GT_RENDERER_GDI);
        CHECK(r != NULL, "renderer GDI");
        GtRenderer* r2 = gtCreateRenderer(w, GT_RENDERER_D2D);
        CHECK(r2 != NULL, "D2D -> fallback GDI, non NULL");
        CHECK(gtCreateRenderer(NULL, GT_RENDERER_GDI) == NULL, "renderer win NULL refuse");
        gtRendererBegin(r);
        gtRendererClear(r, GT_BLACK);
        gtRendererDrawPixel(r, 5, 5, GT_WHITE);
        gtRendererDrawRect(r, 10, 10, 20, 20, GT_RED);
        gtRendererDrawRectLines(r, 40, 10, 20, 20, GT_GREEN);
        gtRendererDrawLine(r, 0, 0, 100, 100, GT_BLUE);
        gtRendererDrawCircle(r, 50, 50, 10, GT_YELLOW);
        gtRendererDrawCircleLines(r, 50, 50, 15, GT_WHITE);
        gtRendererEnd(r);
        CHECK(true, "pipeline renderer complet sans crash");
        gtDestroyRenderer(r2);
        gtDestroyRenderer(r);
        CHECK(true, "destroy renderers");

        /* ---------- 8. Boucle visible + vrai WM_SIZE ---------- */
        begin("8. Boucle + WM_SIZE reel");
        HWND hwnd = FindWindowA("LIBGTWindowClass", "smoke");
        CHECK(hwnd != NULL, "FindWindow retrouve le HWND");
        for (int i = 0; i < 20; i++) {
            gtBeginFrame();
            gtEventsWindow(w);
            gtClearWindow(w, GT_DARKGRAY);
            gtDrawRect(w, 10 + i * 5, 90, 30, 30, GT_RED);
            gtDrawCircle(w, 160, 100, 20 + (i % 10), gtColorRGBA(0x22, 0xC5, 0x5E, 150));
            gtDrawLine(w, i * 10, 0, i * 10, 199, GT_BLUE);
            gtUpdateWindow(w);
            Sleep(16);
        }
        CHECK(gtGetDeltaTime() > 0.0f, "dt actif en boucle");

        /* Vrai resize : SendMessage WM_SIZE -> WndProc -> realloc framebuffer */
        SendMessageA(hwnd, WM_SIZE, 0, MAKELPARAM(400, 300));
        CHECK(gtGetWidth(w) == 400 && gtGetHeight(w) == 300, "resize -> 400x300");
        gtClearWindow(w, GT_RED);
        gtDrawPixel(w, 399, 299, GT_WHITE);   /* coin extreme du NOUVEAU buffer */
        gtUpdateWindow(w);
        CHECK(true, "dessin post-resize sans crash");
        SendMessageA(hwnd, WM_SIZE, 0, MAKELPARAM(0, 0));  /* minimisation */
        CHECK(gtGetWidth(w) == 400, "minimise garde le buffer 400x300");
        end();

        /* ---------- 9. ECS render avec vrai renderer ---------- */
        begin("9. ECS render");
        {
            GtECS ecs;
            gtECSInit(&ecs, 32);
            GtEntity e = gtECSCreateEntity(&ecs);
            gtECSAddTransform(&ecs, e, gtVec2(50, 50), 0, 1);
            GtSprite* spr = gtECSAddSprite(&ecs, e, NULL, GT_GREEN, 0);
            CHECK(spr != NULL, "add sprite");
            spr->size = gtVec2(20, 20);
            GtRenderer* r = gtCreateRenderer(w, GT_RENDERER_GDI);
            gtRendererClear(r, GT_BLACK);
            gtECSSystemRenderSprites(&ecs, r);
            gtRendererEnd(r);
            CHECK(true, "render sprites (rects placeholder) sans crash");
            gtDestroyRenderer(r);
            gtECSDestroy(&ecs);
        }
        end();

        /* ---------- 10. NULL-safety globale ---------- */
        begin("10. NULL-safety");
        {
            gtDestroyWindow(NULL); gtClearWindow(NULL, 0); gtUpdateWindow(NULL);
            gtEventsWindow(NULL); gtGetWidth(NULL); gtGetHeight(NULL);
            CHECK(gtShouldClose(NULL), "shouldClose(NULL)=true");
            CHECK(gtGetWidth(NULL) == 0, "GetWidth(NULL)=0");
            gtDrawPixel(NULL, 0, 0, GT_RED);
            gtDrawRect(NULL, 0, 0, 1, 1, GT_RED);
            gtDrawRectLines(NULL, 0, 0, 1, 1, GT_RED);
            gtDrawLine(NULL, 0, 0, 1, 1, GT_RED);
            gtDrawCircle(NULL, 0, 0, 5, GT_RED);
            gtDrawCircleLines(NULL, 0, 0, 5, GT_RED);
            CHECK(!gtIsKeyDown(NULL, 'W'), "IsKeyDown(NULL)=false");
            CHECK(!gtIsMouseButtonDown(NULL, 0), "Mouse(NULL)=false");
            int mx2, my2;
            gtGetMousePos(NULL, &mx2, &my2);
            gtECSInit(NULL, 8); gtECSDestroy(NULL);
            CHECK(gtECSCreateEntity(NULL) == GT_ENTITY_NULL, "create(NULL)");
            CHECK(gtECSFindFirstByMask(NULL, GT_COMP_PLAYER) == GT_ENTITY_NULL, "find(NULL)");
            CHECK(!gtECSDamageEntity(NULL, 1, 1), "damage(NULL)=false");
            CHECK(gtECSSystemHealthDeath(NULL) == 0, "healthDeath(NULL)=0");
            gtECSSystemVelocity(NULL, 1); gtECSSystemLifetime(NULL, 1);
            gtECSSystemCollisions(NULL, onHit, NULL);
            gtECSSystemRenderSprites(NULL, NULL);
            gtECSRunSystem(NULL, 0, sysCount, NULL);
            gtECSAddTag(NULL, 1, GT_COMP_PLAYER);
            gtRendererBegin(NULL); gtRendererEnd(NULL);
            gtRendererClear(NULL, GT_RED);
            gtRendererDrawRect(NULL, 0, 0, 1, 1, GT_RED);
            gtDestroyRenderer(NULL);
            CHECK(true, "40+ appels NULL sans crash");
        }
        end();

        /* ---------- 11. Input Mapping ---------- */
        begin("11. Input Mapping");
        {
            // Crée système input
            GtInputSystem* input = gtInputCreate();
            CHECK(input != NULL, "gtInputCreate");

            // Test profils
            GtPlayerProfile* p0 = gtInputGetProfile(input, 0);
            CHECK(p0 != NULL, "getProfile(0)");
            CHECK(p0->player_index == 0, "profile index");
            CHECK(p0->stick_deadzone == 0.15f, "default deadzone");
            CHECK(p0->trigger_threshold == 0.1f, "default trigger threshold");

            // Test maps
            GtActionMap* gameplay = gtInputCreateMap("gameplay");
            CHECK(gameplay != NULL, "createMap gameplay");
            CHECK(strcmp(gameplay->name, "gameplay") == 0, "map name");

            GtActionMap* menu = gtInputCreateMap("menu");
            CHECK(menu != NULL, "createMap menu");

            // Test actions
            gtInputAddAction(gameplay, "move_left");
            gtInputAddAction(gameplay, "move_right");
            gtInputAddAction(gameplay, "move_up");
            gtInputAddAction(gameplay, "move_down");
            gtInputAddAction(gameplay, "jump");
            gtInputAddAction(gameplay, "shoot");
            gtInputAddAction(gameplay, "pause");

            CHECK(gameplay->action_count == 7, "7 actions added");

            // Test bindings clavier
            gtInputBindKey(gameplay, "move_left", 'Q');      // AZERTY
            gtInputBindKey(gameplay, "move_left", 'A');      // QWERTY fallback
            gtInputBindKey(gameplay, "move_right", 'D');
            gtInputBindKey(gameplay, "move_up", 'Z');
            gtInputBindKey(gameplay, "move_up", 'W');
            gtInputBindKey(gameplay, "move_down", 'S');
            gtInputBindKey(gameplay, "jump", VK_SPACE);
            gtInputBindKey(gameplay, "shoot", VK_LBUTTON);   // souris gauche
            gtInputBindKey(gameplay, "pause", VK_ESCAPE);

            // Test bindings gamepad (player 0)
            gtInputBindGamepadBtn(gameplay, "jump", 0, XINPUT_GAMEPAD_A);
            gtInputBindGamepadBtn(gameplay, "shoot", 0, XINPUT_GAMEPAD_RIGHT_SHOULDER);
            gtInputBindGamepadAxis(gameplay, "move_left", 0, 0, 0.2f);  // LX neg
            gtInputBindGamepadAxis(gameplay, "move_right", 0, 0, 0.2f); // LX pos (même axis, seuil)
            gtInputBindGamepadAxis(gameplay, "move_up", 0, 1, 0.2f);    // LY neg (haut)
            gtInputBindGamepadAxis(gameplay, "move_down", 0, 1, 0.2f);  // LY pos (bas)

            // Test bindings souris
            gtInputBindMouseBtn(menu, "click", GT_MOUSE_BUTTON_LEFT);
            gtInputBindMouseBtn(menu, "right_click", GT_MOUSE_BUTTON_RIGHT);

            // Test stack maps
            gtInputPushMap(p0, gameplay);
            CHECK(p0->active_map_count == 1, "pushMap count=1");

            gtInputPushMap(p0, menu);  // overlay
            CHECK(p0->active_map_count == 2, "pushMap overlay count=2");

            gtInputPopMap(p0);
            CHECK(p0->active_map_count == 1, "popMap count=1");

            gtInputSetActiveMap(p0, menu);  // clear + push
            CHECK(p0->active_map_count == 1, "setActiveMap count=1");

            gtInputSetActiveMap(p0, gameplay);

            // Test update sans fenêtre (input seulement)
            gtInputUpdate(input, NULL, 0.016f);

            // Test query actions (tout relâché)
            CHECK(!gtInputIsActionPressed(input, 0, "move_left"), "move_left not pressed");
            CHECK(!gtInputIsActionPressed(input, 0, "jump"), "jump not pressed");
            CHECK(gtInputGetActionValue(input, 0, "move_left") == 0.0f, "move_left value=0");

            GtVec2 move = gtInputGetActionVec2(input, 0, "move_right", "move_up");
            CHECK(move.x == 0.0f && move.y == 0.0f, "move vec2 zero");

            // Test rebinding
            GtInputBinding new_bind = {0};
            new_bind.type = GT_INPUT_KEYBOARD;
            new_bind.data.keyboard.vk = VK_SHIFT;
            CHECK(gtInputRebindAction(gameplay, "jump", &new_bind), "rebind jump to SHIFT");

            // Vérifie que l'ancien binding est remplacé
            GtAction* jump_action = gtInputFindAction(gameplay, "jump");
            CHECK(jump_action != NULL, "jump action exists");
            CHECK(jump_action->binding_count == 1, "rebind replaced bindings");
            CHECK(jump_action->bindings[0].type == GT_INPUT_KEYBOARD, "rebind type keyboard");
            CHECK(jump_action->bindings[0].data.keyboard.vk == VK_SHIFT, "rebind vk=SHIFT");

            gtInputClearActionBindings(gameplay, "jump");
            CHECK(jump_action->binding_count == 0, "clear bindings");

            // Test save/load profil
            p0->stick_deadzone = 0.2f;
            p0->mouse_sensitivity = 2.0f;
            gtInputPushMap(p0, gameplay);

            const char* save_path = "test_profile.bin";
            CHECK(gtInputSaveProfile(p0, save_path), "saveProfile");

            GtPlayerProfile loaded = {0};
            CHECK(gtInputLoadProfile(&loaded, save_path), "loadProfile");
            CHECK(loaded.player_index == 0, "loaded player_index");
            CHECK(fabsf(loaded.stick_deadzone - 0.2f) < 0.001f, "loaded deadzone");
            CHECK(fabsf(loaded.mouse_sensitivity - 2.0f) < 0.001f, "loaded sensitivity");
            CHECK(loaded.map_count == 1, "loaded map_count");
            CHECK(strcmp(loaded.maps[0].name, "gameplay") == 0, "loaded map name");
            CHECK(loaded.maps[0].action_count == 7, "loaded action_count");

            // Nettoyage
            gtInputDestroyMap(gameplay);
            gtInputDestroyMap(menu);
            gtInputDestroy(input);

            // Test gamepad bas niveau (ne crash pas si pas de manette)
            CHECK(!gtInputIsGamepadConnected(99), "invalid player index");
            gtInputSetVibration(0, 0.5f, 0.5f, 0.1f); // no crash

            // Test edge detection (pressed/released)
            input = gtInputCreate();
            gameplay = gtInputCreateMap("gameplay");
            gtInputAddAction(gameplay, "test_action");
            gtInputBindKey(gameplay, "test_action", 'X');
            gtInputSetActiveMap(gtInputGetProfile(input, 0), gameplay);

            // Simule frame 1 : rien
            gtInputUpdate(input, NULL, 0.016f);
            CHECK(!gtInputWasActionPressed(input, 0, "test_action"), "not pressed frame 1");
            CHECK(!gtInputWasActionReleased(input, 0, "test_action"), "not released frame 1");

            // Note: on ne peut pas simuler touche enfoncée sans vraie fenêtre
            // Mais l'API est testée structurellement

            gtInputDestroyMap(gameplay);
            gtInputDestroy(input);

            CHECK(true, "Input Mapping complet sans crash");
        }
        end();

        gtDestroyWindow(w);
        CHECK(true, "destroy window final");
    }
    end();

    printf("=== RESULTAT : %d checks, %d echec(s) -> %s ===\n",
           checks, failures, failures ? "ECHEC" : "TOUT PASSE");
    return failures ? 1 : 0;
}