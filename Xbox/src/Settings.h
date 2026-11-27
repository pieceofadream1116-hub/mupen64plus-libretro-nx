#pragma once
// The app's own settings (picture, sound, buttons, folders), saved as a small text file in the profile folder.

#include <filesystem>
#include <string>

/// The picture styles shown in the menu.
enum class PictureStyle
{
	SharpPixels = 0,  // original pixels, no filter
	Smooth,           // original pixels, smoothed when scaled up
	MaxHd,            // edges rounded off (Scale2x twice), then smoothed up to the TV's size
	Count
};

const char* PictureStyleName(PictureStyle style);
const char* PictureStyleHelp(PictureStyle style);

enum class ScreenShape
{
	OriginalTv = 0,   // the shape the game was made for (4:3 on a TV of the time)
	PixelPerfect,     // whole multiples of the picture, square pixels
	FillScreen,       // stretch to the whole TV
	Count
};

const char* ScreenShapeName(ScreenShape shape);

/// The N64 buttons that can be moved to another Xbox button, in AppSettings::buttons order. The C buttons are also
/// always on the right stick.
inline constexpr const char* kPadButtonNames[] = {"A", "B", "Z", "L", "R", "Start", "C-Up", "C-Down", "C-Left", "C-Right"};

/// Buttons that can have turbo: A, B, Z, L and R (the first kTurboCount of kPadButtonNames)
inline constexpr int kTurboCount = 5;

/// "No Xbox button" (a button only on the right stick)
inline constexpr int kNoButton = 63;

struct AppSettings
{
	std::string gamesFolder;                 // extra folder picked by the player (the profile's Games folder is always scanned)
	PictureStyle picture = PictureStyle::Smooth;
	ScreenShape shape = ScreenShape::OriginalTv;
	int scanlines = 0;                       // 0-100
	int resolution = 2;                      // 1 = 640x480, 2 = 1280x960 (HD), 3 = 1920x1440 (full HD and more)
	bool widescreen = false;                 // 16:9
	int volume = 100;                        // 0-100
	int uiScale = 100;                       // text size in percent (100 = big, made for a TV)

	std::string backgroundPicture;           // the player's own picture behind the game list (empty: the moving blue one)

	/// Xbox button (pad::Button number) for each N64 button, in the order of kPadButtonNames.
	/// Default: A = A, X = B, LT = Z, LB/RB = L/R, Menu = Start, Y = C-Up, B = C-Down (all four C buttons are also on
	/// the right stick).
	static constexpr int kButtonCount = 10;
	int buttons[kButtonCount] = {0, 2, 37, 9, 10, 6, 3, 1, 63, 63};
	bool turbo[kTurboCount] = {};            // these buttons fire by themselves while held

	void Load(const std::filesystem::path& file);
	void Save() const;

private:
	std::filesystem::path _file;
};
