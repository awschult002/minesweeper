# Headless grid + camera + gesture + touch bridge (no Orx required)
.PHONY: test clean

CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
INC = -Isrc/game -Isrc/view
SRCS = src/game/mines.c src/view/board_camera.c src/view/gesture.c src/view/touch_bridge.c src/view/menu_layout.c

test: tests/test_mines tests/test_gesture tests/test_menu
	./tests/test_mines
	./tests/test_gesture
	./tests/test_menu

tests/test_mines: tests/test_mines.c src/game/mines.c src/view/board_camera.c src/game/mines.h src/view/board_camera.h
	$(CC) $(CFLAGS) $(INC) -o $@ tests/test_mines.c src/game/mines.c src/view/board_camera.c -lm

tests/test_gesture: tests/test_gesture.c $(SRCS) src/game/mines.h src/view/board_camera.h src/view/gesture.h src/view/touch_bridge.h src/view/menu_layout.h
	$(CC) $(CFLAGS) $(INC) -o $@ tests/test_gesture.c $(SRCS) -lm

tests/test_menu: tests/test_menu.c src/view/menu_layout.c src/view/menu_layout.h
	$(CC) $(CFLAGS) $(INC) -o $@ tests/test_menu.c src/view/menu_layout.c -lm

clean:
	rm -f tests/test_mines tests/test_gesture tests/test_menu
