## Prepare game data with the Windows app

Players using the prebuilt core do not need to compile anything.

1. Download **RetroDriven.CommanderKeen-GamePrep-v1.0.0-Windows.zip** from the release files and extract it on your PC.
2. Open **RetroDriven.CommanderKeen-GamePrep-v1.0.0.exe**.
3. Click **Add folder...** and select your DOS game folders. You can add several locations or one parent folder containing multiple episodes. For Steam Keen Dreams, select **DOS/EGA**.
4. Choose **Prepared files on this PC**, or keep the default **Documents/CommanderKeen-Prepared** folder.
5. Optionally choose the **Pocket SD root**. Install the public core ZIP on the card first. Leave this field blank to prepare files on the PC only.
6. Click **Prepare game data** and wait for success.
7. If you did not select the SD, merge the prepared **Assets** folder into the SD root. The required file is **Assets/keen/common/keenpack.bin**.
8. Safely eject the card before launching Commander Keen.

Include every desired episode each time you prepare a pack: a new pack replaces the old one rather than merging with it. Originals are read without modification. Saves and other cores are preserved. The tool checks files and the completed pack before copying it to the SD.

Use **Required DOS files** inside the app or read [REQUIRED_FILES.md](REQUIRED_FILES.md) for supported filenames, versions and hashes. See [GETTING_THE_GAMES.md](GETTING_THE_GAMES.md) for acquisition links. Logs are stored in **%LOCALAPPDATA%/RetroDriven/CommanderKeenGamePrep-v1/GamePrep.log**.
