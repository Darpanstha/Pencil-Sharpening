# === Pencil Sharpening Championship - Multi-OS Makefile ===

CXX = gcc
CXXFLAGS = -Wall -Wextra -O2

# SDL flags (Linux/macOS)
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)
SDL_IMG = -lSDL2_image
SDL_TTF = -lSDL2_ttf
MAC_FRAMEWORKS = -framework CoreFoundation

SRC = src/main.c
TARGET = pencil
APP_NAME = build/Pencil_Sharpening_Pixel.app
ASSETS_DIR = assets

# macOS paths
APP_CONTENTS = $(APP_NAME)/Contents
APP_MACOS = $(APP_CONTENTS)/MacOS
APP_RESOURCES = $(APP_CONTENTS)/Resources
APP_FRAMEWORKS = $(APP_CONTENTS)/Frameworks

# Detect OS
UNAME_S := $(shell uname)

# === Default build (Linux/macOS) ===
all:
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SRC) -o $(TARGET) \
	$(SDL_LDFLAGS) $(SDL_IMG) $(SDL_TTF) $(MAC_FRAMEWORKS)
else ifeq ($(UNAME_S),Linux)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SRC) -o $(TARGET) $(SDL_LDFLAGS) $(SDL_IMG) $(SDL_TTF)
endif

# === Windows build (MinGW) ===
windows:
	x86_64-w64-mingw32-gcc $(CXXFLAGS) $(SRC) -o $(TARGET).exe \
	-IC:/SDL2/include -LC:/SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

# === macOS .app build ===
mac: all
	@echo "📦 Creating macOS .app bundle..."
	mkdir -p $(APP_MACOS) $(APP_RESOURCES) $(APP_FRAMEWORKS)
	cp $(TARGET) $(APP_MACOS)/
	cp $(ASSETS_DIR)/* $(APP_RESOURCES)/
	if [ -f $(ASSETS_DIR)/icon.icns ]; then cp $(ASSETS_DIR)/icon.icns $(APP_RESOURCES)/; fi

	# Info.plist
	echo '<?xml version="1.0" encoding="UTF-8"?>' > $(APP_CONTENTS)/Info.plist
	echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(APP_CONTENTS)/Info.plist
	echo '<plist version="1.0"><dict>' >> $(APP_CONTENTS)/Info.plist
	echo '  <key>CFBundleExecutable</key><string>pencil</string>' >> $(APP_CONTENTS)/Info.plist
	echo '  <key>CFBundleName</key><string>Pencil Sharpening Championship</string>' >> $(APP_CONTENTS)/Info.plist
	echo '  <key>CFBundleIdentifier</key><string>com.squach90.pencil</string>' >> $(APP_CONTENTS)/Info.plist
	echo '  <key>CFBundleIconFile</key><string>icon.icns</string>' >> $(APP_CONTENTS)/Info.plist
	echo '</dict></plist>' >> $(APP_CONTENTS)/Info.plist

	# Copy SDL dylibs
	cp /opt/homebrew/lib/libSDL2-2.0.0.dylib $(APP_FRAMEWORKS)/
	cp /opt/homebrew/lib/libSDL2_image-2.0.0.dylib $(APP_FRAMEWORKS)/
	cp /opt/homebrew/lib/libSDL2_ttf-2.0.0.dylib $(APP_FRAMEWORKS)/

	# Fix library paths
	install_name_tool -change /opt/homebrew/lib/libSDL2-2.0.0.dylib \
		@executable_path/../Frameworks/libSDL2-2.0.0.dylib \
		$(APP_MACOS)/pencil
	install_name_tool -change /opt/homebrew/lib/libSDL2_image-2.0.0.dylib \
		@executable_path/../Frameworks/libSDL2_image-2.0.0.dylib \
		$(APP_MACOS)/pencil
	install_name_tool -change /opt/homebrew/lib/libSDL2_ttf-2.0.0.dylib \
		@executable_path/../Frameworks/libSDL2_ttf-2.0.0.dylib \
		$(APP_MACOS)/pencil

	@echo "✅ macOS app built successfully: $(APP_NAME)"

# === Clean ===
clean:
	rm -f $(TARGET) $(TARGET).exe
	rm -rf $(APP_NAME)
