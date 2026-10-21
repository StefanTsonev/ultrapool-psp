TARGET = ultrapool
OBJS = src/core/menu_features.o src/core/shop_stock.o src/core/daily.o src/core/daily_history.o src/core/daily_game.o src/core/progression.o src/core/progression_catalog.o src/core/progress_events.o src/core/remaining_effects.o src/core/identity_effects.o src/core/passive_balls.o src/core/rituals.o src/core/collection.o src/core/pocket_events.o src/core/summons.o src/core/holes.o src/core/movement.o src/core/projectiles.o src/core/content_effects.o src/psp/audio.o src/core/effects.o src/core/shop_events.o src/core/decks.o src/core/cubes.o src/core/audio.o src/psp/main.o src/psp/render.o src/psp/input.o src/core/pickups.o src/core/passives.o src/core/splitting.o src/core/ball_events.o src/core/modifiers.o src/core/buffs.o src/core/catalog.o src/core/listeners.o src/core/mixing.o src/core/actions.o src/core/save.o src/core/prediction.o src/core/physics.o src/core/game.o src/core/resources.o src/core/content.o src/core/presentation.o
INCDIR = include
CFLAGS = -O2 -G0 -Wall -Wextra -MMD -MP
CXXFLAGS = $(CFLAGS) -std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBS = -lstdc++ -lpspgu -lpsprtc -lpsppower -lpspaudio -lm
BUILD_PRX = 1
PSP_FW_VERSION = 150
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Ultrapool Native Prototype
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
-include $(OBJS:.o=.d)
